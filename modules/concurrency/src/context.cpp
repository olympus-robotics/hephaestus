//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/context.h"

#include <chrono>
#include <functional>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/concurrency/io_ring/timer.h"

namespace heph::concurrency {
void Context::run(const std::function<void()>& on_start) {
  std::function<bool()> on_progress;
  if (timer_.clockMode() == io_ring::ClockMode::WALLCLOCK) {
    on_progress = [this] { return runTasks(); };
  } else {
    on_progress = [this] { return runTasksSimulated(); };
  }
  start_time_ = io_ring::TimerClock::base_clock::now();
  last_progress_time_ = io_ring::TimerClock::base_clock::now();
  ring_.run(on_start, on_progress);
}

void Context::enqueue(TaskBase* task) {
  if (ring_.stopRequested()) {
    task->setStopped();
    return;
  }
  if (!ring_.isRunning() || ring_.isCurrentRing()) {
    tasks_.push_back(task);
    return;
  }
  ring_.submit(task->dispatch_operation);
}

void Context::enqueueAt(TaskBase* task, io_ring::TimerClock::time_point start_time) {
  if (ring_.stopRequested()) {
    task->setStopped();
    return;
  }
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
  auto now = io_ring::TimerClock::base_clock::now();
  timer_.advanceSimulation(now - last_progress_time_);
  last_progress_time_ = now;

  timer_.tickSimulated(tasks_.empty());

  runTasks();

  if (tasks_.empty() && stopRequested()) {
    return false;
  }
  return true;
}

void Context::runTask(TaskBase* task) {
  if (ring_.stopRequested()) {
    task->setStopped();
  } else {
    task->setValue();
  }
}
}  // namespace heph::concurrency
