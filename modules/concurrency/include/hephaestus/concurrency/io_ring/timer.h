//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/stoppable_io_ring_operation.h"

namespace heph::concurrency {
struct TaskBase;
}
namespace heph::concurrency::io_ring {

enum class ClockMode : std::uint8_t { WALLCLOCK, SIMULATED };

struct TimerOptions {
  ClockMode clock_mode{ ClockMode::WALLCLOCK };
};

class Timer;

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

  friend auto operator<=>(const TimerEntry& lhs, const TimerEntry& rhs) {
    return lhs.start_time <=> rhs.start_time;
  }
};

class Timer {
public:
  explicit Timer(IoRing& ring, TimerOptions options);
  ~Timer() noexcept;

  auto empty() const -> bool {
    return tasks_.empty();
  }

  void requestStop();

  void tick();

  void startAt(TaskBase* task, TimerClock::time_point start_time);
  void dequeue(TaskBase* task);

  auto now() -> TimerClock::time_point {
    return last_tick_;
  }

  auto elapsed() -> TimerClock::duration {
    return TimerClock::now() - start_;
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
    void handleStopped() const;

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
  auto next(bool advance = false) -> TaskBase*;

  friend struct TimerClock;

private:
  IoRing* ring_;
  __kernel_timespec next_timeout_{};
  std::optional<StoppableIoRingOperation<Operation>> timer_operation_;
  std::optional<StoppableIoRingOperation<UpdateOperation>> update_timer_operation_;
  std::vector<TimerEntry> tasks_;

  TimerClock::time_point start_;
  TimerClock::time_point last_tick_;
  ClockMode clock_mode_;
};
}  // namespace heph::concurrency::io_ring
