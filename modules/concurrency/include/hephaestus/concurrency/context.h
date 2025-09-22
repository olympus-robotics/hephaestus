//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <functional>

#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/timer.h"
#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::concurrency {

using TimerOptionsT = io_ring::TimerOptions;
using ClockT = io_ring::TimerClock;

struct ContextConfig {
  io_ring::IoRingConfig io_ring_config;
  TimerOptionsT timer_options;
};

class Context {
public:
  using Scheduler = ContextScheduler;

  explicit Context(const ContextConfig& config)
    : ring_{ config.io_ring_config }
    , timer_{ ring_, config.timer_options }
    , stop_callback_(ring_.getStopToken(), StopCallback{ this }) {
  }

  auto scheduler() -> Scheduler {
    return { this };
  }

  void run(const std::function<void()>& on_start = [] {});

  void requestStop() {
    ring_.requestStop();
  }

  auto isCurrent() -> bool {
    return ring_.isCurrentRing();
  }

  auto stopRequested() -> bool {
    return ring_.stopRequested();
  }

  auto getStopToken() -> stdexec::inplace_stop_token {
    return ring_.getStopToken();
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
  template <typename Receiver, typename Context>
  friend struct TimedTask;

  struct StopCallback {
    Context* self;
    void operator()() const noexcept {
      self->timer_.requestStop();
    }
  };

  void enqueueAt(TaskBase* task, ClockT::time_point start_time);
  void dequeueTimer(TaskBase* task);

  auto runTimedTasks() -> bool;
  auto runTasks() -> bool;
  auto runTasksSimulated() -> bool;

  void runTask(TaskBase* task);

private:
  io_ring::IoRing ring_;
  heph::containers::IntrusiveFifoQueue<TaskBase> tasks_;
  io_ring::Timer timer_;
  ClockT::base_clock::time_point start_time_;
  ClockT::base_clock::time_point last_progress_time_;
  stdexec::inplace_stop_callback<StopCallback> stop_callback_;
};

}  // namespace heph::concurrency
