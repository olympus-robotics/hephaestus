//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <queue>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/stoppable_io_ring_operation.h"

namespace heph::concurrency {
struct TaskBase;

enum class ClockMode : std::uint8_t { WALLCLOCK, SIMULATED };

struct TimerOptions {
  ClockMode clock_mode{ ClockMode::WALLCLOCK };
};

struct Timer;

struct TimerClock {
  using base_clock = std::chrono::steady_clock;

  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;

  using time_point = std::chrono::time_point<TimerClock, duration>;

  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr bool is_steady = base_clock::is_steady;

  static auto now() -> time_point;

  static Timer* timer;
};

struct TimerEntry {
  TaskBase* task{ nullptr };
  TimerClock::time_point start_time;

  friend auto operator<=>(TimerEntry const& lhs, TimerEntry const& rhs) {
    return lhs.start_time <=> rhs.start_time;
  }
};

struct Timer {
  explicit Timer(IoRing& ring, TimerOptions options);

  void requestStop();

  void tick();

  void startAt(TaskBase* task, TimerClock::time_point start_time);

  auto now() -> TimerClock::time_point {
    return last_tick_;
  }

  auto elapsed() -> TimerClock::duration {
    return last_tick_ - start_;
  }

  auto tickSimulated(bool advance) -> bool;
  void advanceSimulation(TimerClock::duration duration) {
    last_tick_ += duration;
  }

  auto clockMode() {
    return clock_mode_;
  }

private:
  struct Operation {
    void prepare(::io_uring_sqe* sqe) const;
    void handleCompletion(::io_uring_cqe* cqe) const;
    void handleStopped();

    Timer* timer{ nullptr };
  };

  struct UpdateOperation {
    void prepare(::io_uring_sqe* sqe) const;
    void handleCompletion(::io_uring_cqe* cqe) const;
    void handleStopped();
    Timer* timer{ nullptr };
    __kernel_timespec next_timeout{};  // NOLINT(misc-include-cleaner)
  };

  void update(TimerClock::time_point start_time);
  auto next() -> TaskBase*;

  IoRing* ring_;
  __kernel_timespec next_timeout_{};
  std::optional<StoppableIoRingOperation<Operation>> timer_operation_;
  std::optional<StoppableIoRingOperation<UpdateOperation>> update_timer_operation_;
  std::priority_queue<TimerEntry, std::deque<TimerEntry>, std::greater<>> tasks_;

  friend struct TimerClock;
  TimerClock::time_point start_;
  TimerClock::time_point last_tick_;
  ClockMode clock_mode_;
};
}  // namespace heph::concurrency
