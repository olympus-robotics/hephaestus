//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/stoppable_io_ring_operation.h"

namespace heph::concurrency {
struct TimedTaskBase {
  virtual ~TimedTaskBase() = default;
  virtual void startTask() noexcept = 0;
};
}  // namespace heph::concurrency
namespace heph::concurrency::io_ring {

enum class ClockMode : std::uint8_t { WALLCLOCK, SIMULATED };

struct TimerOptions {
  ClockMode clock_mode{ ClockMode::WALLCLOCK };
};

class Timer;

struct TimerClock {
  using base_clock = std::chrono::system_clock;

  using duration = std::chrono::microseconds;
  using rep = duration::rep;
  using period = duration::period;

  using time_point = std::chrono::time_point<base_clock, duration>;

  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr bool is_steady = base_clock::is_steady;

  static auto now() -> time_point;

  static absl::Mutex timer_mutex;
  static Timer* timer;
};

struct TimerEntry {
  TimedTaskBase* task{ nullptr };
  TimerClock::time_point start_time;

  friend auto operator<=>(const TimerEntry& lhs, const TimerEntry& rhs) {
    return lhs.start_time <=> rhs.start_time;
  }
};

class Timer {
public:
  explicit Timer(IoRing& ring, TimerOptions options);
  ~Timer() noexcept;

  [[nodiscard]] auto empty() const -> bool {
    const absl::MutexLock lock{ &mutex_ };
    return tasks_.empty();
  }

  void requestStop();

  void tick();

  void startAt(TimedTaskBase* task, TimerClock::time_point start_time);
  void dequeue(TimedTaskBase* task);

  auto now() -> TimerClock::time_point {
    return last_tick_;
  }

  auto elapsed() -> TimerClock::duration {
    return TimerClock::now() - start_;
  }

  auto tickSimulated(bool advance) -> bool;

  template <typename Rep, typename Period>
  void advanceSimulation(std::chrono::duration<Rep, Period> duration) {
    last_tick_ += std::chrono::duration_cast<TimerClock::duration>(duration);
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
  auto next(bool advance = false) -> TimedTaskBase*;

  friend struct TimerClock;

private:
  IoRing* ring_;
  stdexec::inplace_stop_source stop_source_;
  __kernel_timespec next_timeout_{};
  mutable absl::Mutex mutex_;
  std::optional<StoppableIoRingOperation<Operation>> timer_operation_ ABSL_GUARDED_BY(mutex_);
  std::optional<StoppableIoRingOperation<UpdateOperation>> update_timer_operation_ ABSL_GUARDED_BY(mutex_);
  std::vector<TimerEntry> tasks_ ABSL_GUARDED_BY(mutex_);

  TimerClock::time_point start_;
  TimerClock::time_point last_tick_;
  ClockMode clock_mode_;
};
}  // namespace heph::concurrency::io_ring
