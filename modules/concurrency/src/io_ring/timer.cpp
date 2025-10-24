//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/io_ring/timer.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <system_error>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency::io_ring {
absl::Mutex TimerClock::timer_mutex{};
Timer* TimerClock::timer{ nullptr };

auto TimerClock::now() -> time_point {
  absl::MutexLock l{ &TimerClock::timer_mutex };
  if (TimerClock::timer == nullptr) {
    auto now = TimerClock::base_clock::now();
    return time_point{ std::chrono::duration_cast<duration>(now - TimerClock::base_clock::time_point{}) };
  }

  return TimerClock::timer->now();
}

void Timer::Operation::prepare(::io_uring_sqe* sqe) const {
  io_uring_prep_timeout(sqe, &timer->next_timeout_, std::numeric_limits<unsigned>::max(),
                        IORING_TIMEOUT_ETIME_SUCCESS | IORING_TIMEOUT_ABS | IORING_TIMEOUT_REALTIME);
}

void Timer::Operation::handleCompletion(::io_uring_cqe* cqe) const {
  if (cqe->res < 0 && cqe->res != -ETIME) {
    panic("timer failed: {}", std::error_code(-cqe->res, std::system_category()).message());
  }
  {
    absl::MutexLock l{ &timer->mutex_ };
    timer->timer_operation_.reset();
  }

  timer->tick();
}

void Timer::Operation::handleStopped() const {
}

void Timer::UpdateOperation::handleStopped() {
}

void Timer::requestStop() {
  stop_source_.request_stop();

  {
    absl::MutexLock l{ &mutex_ };

    if (timer_operation_.has_value()) {
      timer_operation_->requestStop();
    }
    if (update_timer_operation_.has_value()) {
      update_timer_operation_->requestStop();
    }
  };
}

void Timer::update(TimerClock::time_point start_time) {
  {
    absl::MutexLock l{ &mutex_ };
    auto since_epoch = start_time.time_since_epoch();

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds);
    next_timeout_.tv_sec = seconds.count();
    next_timeout_.tv_nsec = nanoseconds.count();
    if (!timer_operation_.has_value()) {
      timer_operation_.emplace(Operation{ this }, *ring_);
      ring_->submit(&timer_operation_.value());
      return;
    }

    if (update_timer_operation_.has_value()) {
      return;
    }
    update_timer_operation_.emplace(UpdateOperation{ .timer = this, .next_timeout = next_timeout_ }, *ring_);
  }
  ring_->submit(&update_timer_operation_.value());
}

void Timer::UpdateOperation::prepare(::io_uring_sqe* sqe) const {
  absl::MutexLock l{ &timer->mutex_ };
  // NOLINTNEXTLINE(misc-include-cleaner, bugprone-unchecked-optional-access)
  auto* ptr{ &timer->timer_operation_.value() };
  std::uint64_t data{};
  static_assert(sizeof(data) == sizeof(void*));
  std::memcpy(&data, static_cast<void*>(&ptr), sizeof(data));
  ::io_uring_prep_timeout_update(sqe, &timer->next_timeout_, data, IORING_TIMEOUT_ABS);
}

void Timer::UpdateOperation::handleCompletion(::io_uring_cqe* cqe) const {
  if (cqe->res < 0) {
    if (cqe->res == -ENOENT || cqe->res == -EALREADY) {
      timer->tick();
      return;
    }
    panic("timer failed: {}", std::error_code(-cqe->res, std::system_category()).message());
  }

  bool update{ false };
  TimerClock::duration next;
  {
    absl::MutexLock l{ &timer->mutex_ };
    if (timer->next_timeout_.tv_sec != next_timeout.tv_sec ||
        timer->next_timeout_.tv_nsec != next_timeout.tv_nsec) {
      next = std::chrono::duration_cast<TimerClock::duration>(
          std::chrono::seconds(timer->next_timeout_.tv_sec) +
          std::chrono::nanoseconds(timer->next_timeout_.tv_nsec));
      update = true;
    } else {
      timer->update_timer_operation_.reset();
    }
  }
  if (update) {
    timer->update(TimerClock::time_point{} + next);
  }
}

Timer::Timer(IoRing& ring, TimerOptions options)
  : ring_(&ring)
  , start_(std::chrono::duration_cast<TimerClock::duration>(TimerClock::base_clock::now() -
                                                            TimerClock::base_clock::time_point{}))
  , last_tick_(start_)
  , clock_mode_(options.clock_mode) {
  if (clock_mode_ == ClockMode::SIMULATED) {
    absl::MutexLock l{ &TimerClock::timer_mutex };
    TimerClock::timer = this;
  }
}

Timer::~Timer() noexcept {
  absl::MutexLock l{ &TimerClock::timer_mutex };
  TimerClock::timer = nullptr;
}

void Timer::tick() {
  for (TaskBase* task = next(); task != nullptr; task = next()) {
    task->start();
  }
  std::optional<TimerClock::time_point> start_time;
  {
    absl::MutexLock l{ &mutex_ };
    last_tick_ = TimerClock::now();
    if (!tasks_.empty()) {
      const auto& top = tasks_.front();
      start_time.emplace(top.start_time);
    }
  }
  if (start_time.has_value()) {
    update(*start_time);
  }
}

auto Timer::tickSimulated(bool advance) -> bool {
  TaskBase* task{ nullptr };
  {
    absl::MutexLock l{ &mutex_ };
    if (tasks_.empty()) {
      return false;
    }

    if (advance) {
      auto top = tasks_.front();
      std::ranges::pop_heap(tasks_, std::greater<>{});
      tasks_.pop_back();
      if (top.start_time > last_tick_) {
        advanceSimulation(top.start_time - last_tick_);
      }
      task = top.task;
    }
  }
  if (task != nullptr) {
    task->start();
    return !empty();
  }

  task = next();
  if (task != nullptr) {
    task->start();
  }
  return !empty();
}

void Timer::startAt(TaskBase* task, TimerClock::time_point start_time) {
  {
    absl::MutexLock l{ &mutex_ };
    tasks_.emplace_back(task, start_time);
    std::ranges::push_heap(tasks_, std::greater<>{});

    const auto& top = tasks_.front();

    if (top.task != task || clock_mode_ == ClockMode::SIMULATED) {
      return;
    }
  }

  update(start_time);
}

void Timer::dequeue(TaskBase* task) {
  absl::MutexLock l{ &mutex_ };
  auto it = std::ranges::find_if(tasks_, [task](const TimerEntry& entry) { return task == entry.task; });
  if (it == tasks_.end()) {
    return;
  }
  tasks_.erase(it);
  std::ranges::make_heap(tasks_, std::greater<>{});
}

auto Timer::next(bool advance) -> TaskBase* {
  absl::MutexLock l{ &mutex_ };
  if (!tasks_.empty()) {
    auto now = TimerClock::now();
    const auto& top = tasks_.front();
    if (top.start_time <= now) {
      auto* task = top.task;
      if (advance) {
        last_tick_ += (top.start_time - last_tick_);
      }
      std::ranges::pop_heap(tasks_, std::greater<>{});
      tasks_.pop_back();
      return task;
    }
  }
  return nullptr;
}
}  // namespace heph::concurrency::io_ring
