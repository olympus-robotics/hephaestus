//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <functional>

#include <absl/synchronization/mutex.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/timer.h"
#include "hephaestus/containers/intrusive_fifo_queue.h"

/// me
namespace heph::concurrency {

using TimerOptionsT = io_ring::TimerOptions;
using ClockT = io_ring::TimerClock;

/// Test
struct ContextConfig {
  io_ring::IoRingConfig io_ring_config;
  TimerOptionsT timer_options;
};

class Context {
public:
  using Scheduler = ContextScheduler;

  explicit Context(const ContextConfig& config)
    : ring_{ config.io_ring_config }
    , timer_{ ring_, config.timer_options }  //, stop_callback_(ring_.getStopToken(), StopCallback{ this })
  {
  }

  auto scheduler() -> Scheduler {
    return { this };
  }

  void run(const std::function<void()>& on_start = [] {});

  void requestStop() {
    timer_.requestStop();
    ring_.requestStop();
  }

  auto isRunning() -> bool {
    return ring_.isRunning();
  }

  auto isCurrent() -> bool {
    return ring_.isCurrent();
  }

  auto elapsed() {
    return timer_.elapsed();
  }

  auto ring() -> io_ring::IoRing* {
    return &ring_;
  }

private:
  template <typename Receiver, typename Context>
  friend struct Task;
  void enqueue(TaskBase* task);
  void dequeue(TaskBase* task);

  template <typename Receiver, typename Context>
  friend struct TimedTask;

  auto enqueueAt(TaskBase* task, ClockT::time_point start_time) -> bool;
  void dequeueTimer(TaskBase* task);

  auto runTimedTasks() -> bool;
  auto runTasks() -> bool;
  auto runTasksSimulated() -> bool;

private:
  io_ring::IoRing ring_;
  absl::Mutex mutex_;
  heph::containers::IntrusiveFifoQueue<TaskBase> tasks_;
  io_ring::Timer timer_;
  ClockT::base_clock::time_point start_time_;
  ClockT::base_clock::time_point last_progress_time_;
};

}  // namespace heph::concurrency
