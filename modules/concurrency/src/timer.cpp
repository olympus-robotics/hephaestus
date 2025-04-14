//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/timer.h"

#include <cerrno>
#include <chrono>
#include <cmath>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "hephaestus/concurrency/context.h"

namespace heph::concurrency {
void Timer::Operation::prepare(::io_uring_sqe* sqe) {
  io_uring_prep_timeout(sqe, &timer->next_timeout_, 0, IORING_TIMEOUT_ETIME_SUCCESS | IORING_TIMEOUT_ABS);
}

void Timer::Operation::handleCompletion(::io_uring_cqe* cqe) {
  if (cqe->res < 0 && cqe->res != -ETIME) {
    heph::panic(
        fmt::format("timer failed: {}", std::error_code(-cqe->res, std::system_category()).message()));
  }
  timer->timer_operation_.reset();

  timer->tick();
}

void Timer::Operation::handleStopped() {
}

void Timer::UpdateOperation::handleStopped() {
}

void Timer::requestStop() {
  if (timer_operation_.has_value()) {
    timer_operation_->requestStop();
  }
  if (update_timer_operation_.has_value()) {
    update_timer_operation_->requestStop();
  }
  while (!tasks_.empty()) {
    auto entry = tasks_.top();
    tasks_.pop();

    entry.task->setStopped();
  }
}

void Timer::update(std::chrono::steady_clock::time_point start_time) {
  auto since_epoch = start_time.time_since_epoch();

  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
  auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds);
  next_timeout_.tv_sec = seconds.count();
  next_timeout_.tv_nsec = nanoseconds.count();
  if (!timer_operation_.has_value()) {
    timer_operation_.emplace(Operation{ this }, *ring_, ring_->getStopToken());
    ring_->submit(timer_operation_.value());
    return;
  }

  if (update_timer_operation_.has_value()) {
    return;
  }
  update_timer_operation_.emplace(UpdateOperation{ .timer = this, .next_timeout = next_timeout_ }, *ring_,
                                  ring_->getStopToken());
  ring_->submit(update_timer_operation_.value());
}

void Timer::UpdateOperation::prepare(::io_uring_sqe* sqe) {
  IoRingOperationPointer ptr{ &timer->timer_operation_.value() };
  ::io_uring_prep_timeout_update(sqe, &timer->next_timeout_, ptr.data, IORING_TIMEOUT_ABS);
}

void Timer::UpdateOperation::handleCompletion(::io_uring_cqe* cqe) {
  if (cqe->res < 0) {
    if (cqe->res == -ENOENT) {
      timer->tick();
      return;
    }
    heph::panic(
        fmt::format("timer failed: {}", std::error_code(-cqe->res, std::system_category()).message()));
  }
  if (timer->next_timeout_.tv_sec != next_timeout.tv_sec ||
      timer->next_timeout_.tv_nsec != next_timeout.tv_nsec) {
    auto next = std::chrono::seconds(timer->next_timeout_.tv_sec) +
                std::chrono::nanoseconds(timer->next_timeout_.tv_nsec);
    timer->update(std::chrono::steady_clock::time_point{} + next);
    return;
  }
  timer->update_timer_operation_.reset();
}

Timer::Timer(IoRing& ring) : ring_(&ring) {
}

void Timer::tick() {
  for (TaskBase* task = next(); task != nullptr; task = next()) {
    task->start();
  }

  if (!tasks_.empty()) {
    auto const& top = tasks_.top();
    update(top.start_time);
  }
}

void Timer::startAfter(TaskBase* task, std::chrono::steady_clock::duration start_after) {
  auto start_time = std::chrono::steady_clock::now() + start_after;

  tasks_.emplace(task, start_time);

  auto const& top = tasks_.top();
  if (top.task != task) {
    return;
  }
  update(start_time);
}

auto Timer::next() -> TaskBase* {
  if (!tasks_.empty()) {
    auto now = std::chrono::steady_clock::now();
    auto const& top = tasks_.top();
    if (top.start_time <= now) {
      auto* task = top.task;
      tasks_.pop();
      return task;
    }
  }
  return nullptr;
}
}  // namespace heph::concurrency
