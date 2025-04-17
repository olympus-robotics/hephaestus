//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <utility>

#include <absl/synchronization/mutex.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/concurrency/io_ring.h"
#include "hephaestus/concurrency/timer.h"

namespace heph::concurrency {
struct Context;

struct ContextConfig {
  IoRingConfig io_ring_config;
  TimerOptions timer_options;
};

struct Context {
  using Scheduler = ContextScheduler;

  explicit Context(ContextConfig const& config)
    : ring_{ config.io_ring_config }, timer_{ ring_, config.timer_options } {
  }

  auto scheduler() -> Scheduler {
    return { this };
  }

  void run(const std::function<void()>& on_start = [] {});

  void requestStop() {
    timer_.requestStop();
    ring_.requestStop();
  }

  auto getStopToken() -> stdexec::inplace_stop_token {
    return ring_.getStopToken();
  }

  auto elapsed() {
    return timer_.elapsed();
  }

  void enqueue(TaskBase* task);
  void enqueueAt(TaskBase* task, TimerClock::time_point start_time);

private:
  IoRing ring_;
  absl::Mutex tasks_mutex_;
  std::deque<TaskBase*> tasks_;
  Timer timer_;
  TimerClock::base_clock::time_point start_time_;
  TimerClock::base_clock::time_point last_progress_time_;

  auto runTimedTasks() -> bool;
  auto runTasks() -> bool;
  auto runTasksSimulated() -> bool;

  void runTask(TaskBase* task);
};

}  // namespace heph::concurrency
