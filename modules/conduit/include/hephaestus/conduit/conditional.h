//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <type_traits>
#include <utility>

#include <absl/synchronization/mutex.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/stoppable_operation_state.h"
#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::conduit {

/// `Conditional` is triggering once enabled and blocking execution if disabled.
/// \ref enable and \ref disable are called externally. After construction, it is
/// disabled.
class Conditional : public BasicInput {
public:
  Conditional() noexcept : BasicInput("conditional") {
  }

  void enable() {
    containers::IntrusiveFifoQueue<OperationBase> waiters;

    // NOTE: this is a little hack to avoid recursive mutexes when enabling the node.
    static thread_local bool enabling = false;

    if (enabling) {
      return;
    }

    {
      const absl::MutexLock lock{ &mtx_ };

      if (!enabled_ && node() != nullptr) {
        enabling = true;
        node()->enable();
        enabling = false;
      }

      enabled_ = true;
      std::swap(waiters, waiters_);
    }

    // Once we get enabled, trigger all pending waiters...
    while (true) {
      auto* waiter = waiters.dequeue();
      if (waiter == nullptr) {
        return;
      }
      waiter->restart();
    }
  }

  /// \throws heph::Panic if already disabled
  void disable() {
    // NOTE: this is a little hack to avoid recursive mutexes when disabling the node.
    static thread_local bool disabling = false;

    if (disabling) {
      return;
    }
    const absl::MutexLock lock{ &mtx_ };

    if (enabled_ && node() != nullptr) {
      disabling = true;
      node()->disable();
      disabling = false;
    }

    enabled_ = false;
  }

private:
#ifndef DOXYGEN
  struct OperationBase {
    virtual ~OperationBase() = default;
    virtual void restart() noexcept = 0;
    [[maybe_unused]] OperationBase* next{ nullptr };
    [[maybe_unused]] OperationBase* prev{ nullptr };
  };

  struct ConditionalWaiter {
    using sender_concept = stdexec::sender_t;
    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(bool), stdexec::set_stopped_t()>;

    template <typename Receiver>
    class Operation : public OperationBase {
    public:
      Operation(Conditional* self, Receiver receiver)
        : self_(self), operation_state_(std::move(receiver), [this]() { setStopped(); }) {
      }

      void start() noexcept {
        auto start_transition = operation_state_.start();

        trigger();
      }

      void restart() noexcept final {
        auto start_transition = operation_state_.restart();
        trigger();
      }

    private:
      void trigger() {
        {
          const absl::MutexLock lock{ &self_->mtx_ };
          if (!self_->enabled_) {
            self_->waiters_.enqueue(this);
            return;
          }
        }
        operation_state_.setValue(true);
      }

      void setStopped() {
        const absl::MutexLock lock{ &self_->mtx_ };
        self_->waiters_.erase(this);
      }

    private:
      Conditional* self_{ nullptr };
      concurrency::StoppableOperationState<Receiver, bool> operation_state_;
    };

    template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
    auto connect(Receiver&& receiver) {
      return Operation<ReceiverT>{ self, std::forward<Receiver>(receiver) };
    }
    Conditional* self;
  };
#endif
  auto doTrigger(SchedulerT /*scheduler*/) -> SenderT final {
    return ConditionalWaiter{ this };
  }

  void handleCompleted() final {
    // Do nothing ...
  }

private:
  absl::Mutex mtx_;
  bool enabled_{ true };

  containers::IntrusiveFifoQueue<OperationBase> waiters_;
};
}  // namespace heph::conduit
