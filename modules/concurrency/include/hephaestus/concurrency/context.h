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

struct ContextConfig {
  io_ring::IoRingConfig io_ring_config;
  io_ring::TimerOptions timer_options;
};

class Context {
public:
  using Scheduler = ContextScheduler;

  explicit Context(const ContextConfig& config)
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
  void enqueueAt(TaskBase* task, io_ring::TimerClock::time_point start_time);

  auto runTimedTasks() -> bool;
  auto runTasks() -> bool;
  auto runTasksSimulated() -> bool;

  void runTask(TaskBase* task);

private:
  io_ring::IoRing ring_;
  heph::containers::IntrusiveFifoQueue<TaskBase> tasks_;
  io_ring::Timer timer_;
  io_ring::TimerClock::base_clock::time_point start_time_;
  io_ring::TimerClock::base_clock::time_point last_progress_time_;
};

}  // namespace heph::concurrency
