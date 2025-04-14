//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <queue>

#include "hephaestus/concurrency/io_ring.h"
#include "hephaestus/concurrency/stoppable_io_ring_operation.h"

namespace heph::concurrency {
struct TaskBase;

struct Timer {
  explicit Timer(IoRing& ring);

  void requestStop();

  void tick();

  void startAfter(TaskBase* task, std::chrono::steady_clock::duration start_after);

private:
  struct Operation {
    void prepare(::io_uring_sqe* sqe);
    void handleCompletion(::io_uring_cqe* cqe);
    void handleStopped();

    Timer* timer;
  };

  struct UpdateOperation {
    void prepare(::io_uring_sqe* sqe);
    void handleCompletion(::io_uring_cqe* cqe);
    void handleStopped();
    Timer* timer;
    __kernel_timespec next_timeout{};
  };

  struct TimerEntry {
    TaskBase* task{ nullptr };
    std::chrono::steady_clock::time_point start_time;

    friend auto operator<=>(TimerEntry const& lhs, TimerEntry const& rhs) {
      return lhs.start_time <=> rhs.start_time;
    }
  };

  void update(std::chrono::steady_clock::time_point start_time);
  auto next() -> TaskBase*;

  IoRing* ring_;
  __kernel_timespec next_timeout_{};
  std::optional<StoppableIoRingOperation<Operation>> timer_operation_;
  std::optional<StoppableIoRingOperation<UpdateOperation>> update_timer_operation_;
  std::priority_queue<TimerEntry, std::deque<TimerEntry>, std::greater<>> tasks_;
};
}  // namespace heph::concurrency
