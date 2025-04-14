//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/context.h"

namespace heph::concurrency {

namespace context_internal {
[[nodiscard]] inline auto Env::query(stdexec::get_stop_token_t /*ignore*/) const noexcept
    -> stdexec::inplace_stop_token {
  return self->getStopToken();
}

}  // namespace context_internal

void TaskDispatchOperation::handleCompletion() const {
  self->start();
}

void Context::enqueue(TaskBase* task) {
  if (!ring_.isRunning() || ring_.isCurrentRing()) {
    tasks_.push_back(task);
    return;
  }
  ring_.submit(task->dispatch_operation);
}

void Context::enqueueAfter(TaskBase* task, std::chrono::steady_clock::duration start_after) {
  timer_.startAfter(task, start_after);
}

void Context::runTasks() {
  std::deque<TaskBase*> tasks;
  std::swap(tasks, tasks_);
  while (!tasks.empty()) {
    TaskBase* task = tasks.front();
    tasks.pop_front();
    task->setValue();
  }
}
}  // namespace heph::concurrency
