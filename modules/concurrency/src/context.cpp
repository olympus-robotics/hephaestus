//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/context.h"

#include <chrono>

#include <fmt/chrono.h>
#include <fmt/ostream.h>

namespace heph::concurrency {
void Context::run(const std::function<void()>& on_start) {
  std::function<bool()> on_progress;
  if (timer_.clockMode() == ClockMode::WALLCLOCK) {
    on_progress = [this] {
      bool more_work = timer_.tick();
      return runTasks() || more_work;
    };
  } else {
    on_progress = [this] { return runTasksSimulated(); };
  }
  start_time_ = TimerClock::base_clock::now();
  last_progress_time_ = TimerClock::base_clock::now();
  ring_.run(on_start, on_progress);
}

void Context::enqueue(TaskBase* task) {
  if (!ring_.isRunning() || ring_.isCurrentRing()) {
    tasks_.push_back(task);
    return;
  }
  ring_.submit(task->dispatch_operation);
}

void Context::enqueueAt(TaskBase* task, TimerClock::time_point start_time) {
  if (!ring_.isRunning() || ring_.isCurrentRing()) {
    timer_.startAt(task, start_time);
    return;
  }
  ring_.submit(task->dispatch_operation);
}

auto Context::runTasks() -> bool {
  if (tasks_.empty()) {
    return false;
  }

  TaskBase* task = tasks_.front();
  tasks_.pop_front();
  runTask(task);

  return !tasks_.empty();
}

auto Context::runTasksSimulated() -> bool {
  auto now = TimerClock::base_clock::now();
  timer_.advanceSimulation(now - last_progress_time_);
  last_progress_time_ = now;

  bool progress = timer_.tickSimulated(tasks_.empty());

  return runTasks() || progress;
}

void Context::runTask(TaskBase* task) {
  if (ring_.stopRequested()) {
    task->setStopped();
  } else {
    task->setValue();
  }
}
}  // namespace heph::concurrency
