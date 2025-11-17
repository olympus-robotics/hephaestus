//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <type_traits>
#include <utility>

#include <absl/synchronization/mutex.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>
#include <stdexec/stop_token.hpp>

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
      Operation(Conditional* self, Receiver receiver) : self_(self), receiver_(std::move(receiver)) {
      }

      void start() noexcept {
        auto stop_token = stdexec::get_stop_token(stdexec::get_env(receiver_));
        if (stop_token.stop_requested()) {
          stdexec::set_stopped(std::move(receiver_));
          return;
        }

        if (!trigger()) {
          stop_token_callback_.emplace(stop_token, StopCallback{ this });
        }
      }

      void restart() noexcept final {
        trigger();
      }

    private:
      auto trigger() -> bool {
        {
          const absl::MutexLock lock{ &self_->mtx_ };
          if (!self_->enabled_) {
            self_->waiters_.enqueue(this);
            return false;
          }
          reset();
        }
        stdexec::set_value(std::move(receiver_), true);

        return true;
      }

      void reset() {
        stop_token_callback_.reset();
        self_->waiters_.erase(this);
      }

      void setStopped() {
        {
          const absl::MutexLock lock{ &self_->mtx_ };
          reset();
        }
        stdexec::set_stopped(std::move(receiver_));
      }

    private:
      struct StopCallback {
        Operation* self;
        void operator()() const {
          self->setStopped();
        }
      };
      using ReceiverEnvT = stdexec::env_of_t<Receiver>;
      using StopTokenT = stdexec::stop_token_of_t<ReceiverEnvT>;
      using StopTokenCallbackT = stdexec::stop_callback_for_t<StopTokenT, StopCallback>;

      Conditional* self_{ nullptr };
      Receiver receiver_;
      std::optional<StopTokenCallbackT> stop_token_callback_;
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
