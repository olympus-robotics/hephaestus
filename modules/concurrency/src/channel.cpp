//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================
//
#include "hephaestus/concurrency/channel.h"

#include <atomic>

#include <absl/synchronization/mutex.h>

#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency::internal {
void AwaiterBase::start() noexcept {
  if (!startImpl()) {
    finalizeStart();
  }
}

void AwaiterBase::retry() noexcept {
  // retry might be called concurrently to start. Wait until starting completed.
  waitForStarting();

  // Account for potentially concurrent stop signals arriving...
  if (isStopped()) {
    setStopped();
    return;
  }

  retryImpl();
}

void AwaiterBase::stopRequested() {
  // If we were able to erase ourself from the queue, the only thing left to do
  // is to ensure that we are in the correct state. Possible current states are:
  //  - STARTING: We are called concurrently with start, possibly on the same
  //              thread, set stopped and return, the start function will take
  //              care of stopping.
  //  - ENQUEUED: The awaiter was enqueued, since we were able to erase the awaiter,
  //              we can just call set_stopped.
  auto state = state_.exchange(QueueAwaiterState::STOPPED, std::memory_order_acq_rel);

  if (state == QueueAwaiterState::ENQUEUED) {
    setStopped();
    return;
  }
  HEPH_PANIC_IF(state == QueueAwaiterState::STOPPED, "Stop called multiple times");
}

void AwaiterBase::finalizeStart() {
  // Transition to enqueued state. If the previous state was stopped, we need to handle
  // this here.
  auto state = state_.exchange(QueueAwaiterState::ENQUEUED, std::memory_order_acq_rel);
  if (state == QueueAwaiterState::STOPPED) {
    setStopped();
    return;
  }
  // If we weren't stopped, notify potential waiters...
  state_.notify_all();
}

void AwaiterBase::waitForStarting() const {
  while (state_.load(std::memory_order_acquire) == QueueAwaiterState::STARTING) {
    state_.wait(QueueAwaiterState::STARTING, std::memory_order_acquire);
  }
}

auto AwaiterBase::isStopped() const -> bool {
  return state_.load(std::memory_order_acquire) == QueueAwaiterState::STOPPED;
}

void AwaiterBase::OnStopRequested::operator()() const noexcept {
  if (queue->erase(self)) {
    self->stopRequested();
  }
}

void AwaiterQueue::retryNext() {
  auto* awaiter = [this]() {
    const absl::MutexLock lock{ &mutex_ };
    return queue_.dequeue();
  }();
  if (awaiter != nullptr) {
    awaiter->retry();
  }
}

auto AwaiterQueue::erase(AwaiterBase* awaiter) -> bool {
  const absl::MutexLock lock{ &mutex_ };
  return queue_.erase(awaiter);
}

void AwaiterQueue::enqueue(AwaiterBase* awaiter) {
  const absl::MutexLock lock{ &mutex_ };
  queue_.enqueue(awaiter);
}
}  // namespace heph::concurrency::internal
