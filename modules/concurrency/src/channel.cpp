//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================
//
#include "hephaestus/concurrency/channel.h"

#include <atomic>
#include <thread>

#include <absl/synchronization/mutex.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/format/generic_formatter.h"

namespace heph::concurrency::internal {
void AwaiterBase::start() noexcept {
  if (!startImpl()) {
    emplaceStopCallback();
    finalizeStart();
  }
}

void AwaiterBase::retry() noexcept {
  // retry might be called concurrently to start. Wait until starting completed.
  // Account for potentially concurrent stop signals arriving
  if (!waitForEnqueued()) {
    return;
  }
  resetStopCallback();

  // reset the state to STARTING to make sure that concurrently running stop operations
  // will not attempt to use the (tiny) window between re-enqueuing and resetting the
  // stop callback!
  auto state = state_.exchange(QueueAwaiterState::STARTING, std::memory_order_acq_rel);
  HEPH_PANIC_IF(state != QueueAwaiterState::ENQUEUED, "Invalid State. Got {}, expected {}", state,
                QueueAwaiterState::ENQUEUED);

  if (!retryImpl()) {
    // If we didn't succeed, reinstantiate the stop callback to ensure we are
    // not missing any...
    emplaceStopCallback();
    finalizeStart();
  }
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
  }
}

auto AwaiterBase::waitForEnqueued() const -> bool {
  // Wait for state to transition to started (or stopped...)
  while (true) {
    auto state = state_.load(std::memory_order_acquire);
    if (state != QueueAwaiterState::STARTING) {
      return state == QueueAwaiterState::ENQUEUED;
    }
    std::this_thread::yield();
  }
  return true;
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
