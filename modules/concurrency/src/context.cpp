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
  start_time_ = ClockT::base_clock::now();
  last_progress_time_ = ClockT::base_clock::now();
  ring_.run(on_start, on_progress);
}

void Context::enqueue(TaskBase* task) {
  {
    absl::MutexLock l{ &mutex_ };
    tasks_.enqueue(task);
  }
  ring_.notify();
}

void Context::dequeue(TaskBase* task) {
  absl::MutexLock l{ &mutex_ };
  tasks_.erase(task);
}

auto Context::enqueueAt(TaskBase* task, ClockT::time_point start_time) -> bool {
  if (!ring_.isRunning() || ring_.isCurrent()) {
    timer_.startAt(task, start_time);
    return true;
  }
  ring_.submit(&task->dispatch_operation);
  return false;
}

void Context::dequeueTimer(TaskBase* task) {
  dequeue(task);
  timer_.dequeue(task);
}

auto Context::runTasks() -> bool {
  TaskBase* task{ nullptr };
  {
    absl::MutexLock l{ &mutex_ };
    if (tasks_.empty()) {
      return false;
    }

    task = tasks_.dequeue();
  }
  task->setValue();

  {
    absl::MutexLock l{ &mutex_ };
    return !tasks_.empty();
  }
}

auto Context::runTasksSimulated() -> bool {
  auto now = ClockT::base_clock::now();
  timer_.advanceSimulation(now - last_progress_time_);
  last_progress_time_ = now;

  // FIXME: make thread safe
  timer_.tickSimulated(tasks_.empty() && !ring_.hasWork());

  runTasks();

  if (tasks_.empty() && timer_.empty() && !ring_.isRunning()) {
    return false;
  }
  return true;
}
}  // namespace heph::concurrency
