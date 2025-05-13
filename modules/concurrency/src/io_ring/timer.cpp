//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/io_ring/timer.h"

#include <cerrno>
#include <chrono>
#include <limits>
#include <system_error>

#include <fmt/format.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_pointer.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::io_ring {
Timer* TimerClock::timer{ nullptr };
auto TimerClock::now() -> time_point {
  switch (timer->clock_mode_) {
    case ClockMode::WALLCLOCK: {
      auto now = TimerClock::base_clock::now();
      return time_point{ now - TimerClock::base_clock::time_point{} };
    }
    case ClockMode::SIMULATED:
      return timer->now();
  }
}

void Timer::Operation::prepare(::io_uring_sqe* sqe) const {
  io_uring_prep_timeout(sqe, &timer->next_timeout_, std::numeric_limits<unsigned>::max(),
                        IORING_TIMEOUT_ETIME_SUCCESS | IORING_TIMEOUT_ABS);
}

void Timer::Operation::handleCompletion(::io_uring_cqe* cqe) const {
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

void Timer::update(TimerClock::time_point start_time) {
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

void Timer::UpdateOperation::prepare(::io_uring_sqe* sqe) const {
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const IoRingOperationPointer ptr{ &timer->timer_operation_.value() };
  ::io_uring_prep_timeout_update(sqe, &timer->next_timeout_, ptr.data, IORING_TIMEOUT_ABS);
}

void Timer::UpdateOperation::handleCompletion(::io_uring_cqe* cqe) const {
  if (cqe->res < 0) {
    if (cqe->res == -ENOENT || cqe->res == -EALREADY) {
      timer->tick();
      return;
    }
    heph::panic(
        fmt::format("timer failed: {}", std::error_code(-cqe->res, std::system_category()).message()));
  }
  if (timer->next_timeout_.tv_sec != next_timeout.tv_sec ||
      timer->next_timeout_.tv_nsec != next_timeout.tv_nsec) {
    auto next = std::chrono::duration_cast<TimerClock::duration>(
        std::chrono::seconds(timer->next_timeout_.tv_sec) +
        std::chrono::nanoseconds(timer->next_timeout_.tv_nsec));
    timer->update(TimerClock::time_point{} + next);
    return;
  }
  timer->update_timer_operation_.reset();
}

Timer::Timer(IoRing& ring, TimerOptions options)
  : ring_(&ring)
  , start_(TimerClock::base_clock::now() -
           TimerClock::base_clock::time_point{})  //(std::chrono::system_clock::now() -
                                                  // std::chrono::system_clock::time_point{})
  , last_tick_(start_)
  , clock_mode_(options.clock_mode) {
  TimerClock::timer = this;
}

void Timer::tick() {
  for (TaskBase* task = next(); task != nullptr; task = next()) {
    task->start();
  }
  last_tick_ = TimerClock::now();
  if (!tasks_.empty()) {
    auto const& top = tasks_.top();
    update(top.start_time);
  }
}

auto Timer::tickSimulated(bool advance) -> bool {
  if (tasks_.empty()) {
    return false;
  }

  if (advance) {
    auto const& top = tasks_.top();
    if (top.start_time > last_tick_) {
      advanceSimulation(top.start_time - last_tick_);
    }
    top.task->start();
    tasks_.pop();
    return !tasks_.empty();
  }

  TaskBase* task = next();
  if (task != nullptr) {
    task->start();
  }
  return !tasks_.empty();
}

void Timer::startAt(TaskBase* task, TimerClock::time_point start_time) {
  tasks_.emplace(task, start_time);

  auto const& top = tasks_.top();

  if (top.task != task || clock_mode_ == ClockMode::SIMULATED) {
    return;
  }

  update(start_time);
}

auto Timer::next(bool advance) -> TaskBase* {
  if (!tasks_.empty()) {
    auto now = TimerClock::now();
    auto const& top = tasks_.top();
    if (top.start_time <= now) {
      auto* task = top.task;
      if (advance) {
        last_tick_ += (top.start_time - last_tick_);
      }
      tasks_.pop();
      return task;
    }
  }
  return nullptr;
}
}  // namespace heph::concurrency::io_ring
