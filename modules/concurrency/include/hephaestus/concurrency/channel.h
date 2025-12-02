//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <absl/base/thread_annotations.h>
#include <boost/circular_buffer.hpp>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::concurrency {
namespace internal {
struct AwaiterBase {
  virtual ~AwaiterBase() = default;
  virtual void retry() noexcept = 0;

private:
  friend struct heph::containers::IntrusiveFifoQueueAccess;
  [[maybe_unused]] AwaiterBase* next_{ nullptr };
  [[maybe_unused]] AwaiterBase* prev_{ nullptr };
};

using AwaiterQueueT = containers::IntrusiveFifoQueue<AwaiterBase>;

template <typename T, std::size_t Capacity>
struct GetValueSender;
template <typename T, std::size_t Capacity>
struct SetValueSender;
}  // namespace internal

template <typename T>
concept ChannelValueType = requires(T t) {
  requires std::is_nothrow_destructible_v<T>;
  requires std::is_nothrow_move_assignable_v<T>;
  requires std::is_nothrow_move_constructible_v<T>;
};

/// Asynchronous communication channel
///
/// The member functions \ref getValue and \ref setValue return senders.
/// For waiting on them, use `stdexec::sync_wait`.
///
/// @tparam T The type of the values to store
/// @tparam Capacity Maximum number of elements the Channel can store
///
/// Exception Safety: cannot throw, ensured by type constraints.
///
/// Example:
/// @code
/// Channel<int, 10> c;
///
/// // Using Sender/Receiver
/// // Producer:
/// c.setValue(42) | stdexec::then([](){ fmt::println("Value has been successfully set!"); });
///
/// // Consumer:
/// c.getValue() | stdexec::then([](int res){ fmt::println("Received {}!", res); });
///
/// // Using `stdexec::sync_wait`
/// // Producer:
/// stdexec::sync_wait(c.setValue(42));
///
/// // Consumer:
/// auto [res] = *stdexec::sync_wait(c.getValue());
/// assert(res == 42);
///
/// // Using coroutines:
/// // Producer:
/// co_await c.setValue(42);
///
/// // Consumer:
/// auto res = co_wait c.getValue();
/// assert(res == 42);
///
/// @endcode
template <ChannelValueType T, std::size_t Capacity>
class Channel {
  using GetValueSender = internal::GetValueSender<T, Capacity>;
  using SetValueSender = internal::SetValueSender<T, Capacity>;

public:
  /// Retrieve a value stored in the channel. The returned sender will complete as soon as there is
  /// at least one item stored in the channel.
  ///
  /// @param t The value to send via the channel
  template <ChannelValueType U>
  [[nodiscard]] auto setValue(U&& value) noexcept -> SetValueSender;
  /// Push a value into the channel. The returned sender will complete if there is space to store an
  /// element. Otherwise blocks until at least one item was consumed. Then returned sender completes with the
  /// value received from the channel.
  [[nodiscard]] auto getValue() noexcept -> GetValueSender;

  [[nodiscard]] auto tryGetValue() noexcept -> std::optional<T> {
    internal::AwaiterBase* set_awaiter{ nullptr };
    std::optional<T> res;
    {
      std::scoped_lock lock{ mutex_ };
      if (data_.empty()) {
        return res;
      }

      res.emplace(std::move(data_.front()));
      data_.pop_front();

      set_awaiter = set_awaiters_.dequeue();
    }
    if (set_awaiter != nullptr) {
      set_awaiter->retry();
    }
    return res;
  }

  void setValueOverwrite(T&& t) noexcept {
    internal::AwaiterBase* get_awaiter{ nullptr };
    {
      std::scoped_lock lock{ mutex_ };
      if (data_.full()) {
        data_.pop_front();
      }
      data_.push_back(std::move(t));

      get_awaiter = get_awaiters_.dequeue();
    }
    if (get_awaiter != nullptr) {
      get_awaiter->retry();
    }
  }

private:
  template <typename T_, std::size_t Capacity_>
  friend struct internal::SetValueSender;
  template <typename T_, std::size_t Capacity_>
  friend struct internal::GetValueSender;

  template <typename OnSuccess, typename OnFail>
  void setValueImpl(T&& t, internal::AwaiterBase* set_awaiter, OnSuccess on_success,
                    OnFail on_fail) noexcept {
    internal::AwaiterBase* get_awaiter{ nullptr };
    {
      std::scoped_lock lock{ mutex_ };
      if (data_.full()) {
        set_awaiters_.enqueue(set_awaiter);
        on_fail();
        return;
      }
      data_.push_back(std::move(t));

      get_awaiter = get_awaiters_.dequeue();
      on_success();
    }
    if (get_awaiter != nullptr) {
      get_awaiter->retry();
    }
  }

  template <typename OnSuccess, typename OnFail>
  void getValueImpl(internal::AwaiterBase* get_awaiter, OnSuccess on_success, OnFail on_fail) noexcept {
    internal::AwaiterBase* set_awaiter{ nullptr };
    {
      std::scoped_lock lock{ mutex_ };
      if (data_.empty()) {
        get_awaiters_.enqueue(get_awaiter);
        on_fail();
        return;
      }
      on_success(std::move(data_.front()));
      data_.pop_front();

      set_awaiter = set_awaiters_.dequeue();
    }
    if (set_awaiter != nullptr) {
      set_awaiter->retry();
    }
  }

private:
  // NOTE: The recursive lock is currently required to ensure proper lifetime in the presence
  // of stop requests being signaled concurrently from a different thread. If either setting
  // or getting value fails, we have to construct the stop callback under the lock to ensure
  // proper ordering between threads. On the occasion that a stop request was already signaled,
  // the callback will be invoked immediately. Stopping however requires the same lock to be
  // acquired.
  // If the lock would be released right before constructing the callback, we could get a race
  // between construction and destruction as the stop callback is constructed, while another
  // thread could complete the same sender with a completion signal.
  mutable std::recursive_mutex mutex_;
  boost::circular_buffer<T> data_{ Capacity };
  internal::AwaiterQueueT set_awaiters_ ABSL_GUARDED_BY(mutex_);
  internal::AwaiterQueueT get_awaiters_ ABSL_GUARDED_BY(mutex_);
};

namespace internal {
template <typename T, std::size_t Capacity>
struct SetValueSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t()>;

  template <typename Receiver>
  class Operation : public AwaiterBase {
    struct OnStopRequested {
      void operator()() const noexcept {
        bool call_stop = [this]() {
          std::scoped_lock lock{ channel->mutex_ };
          if (channel->set_awaiters_.erase(self)) {
            return true;
          }
          return false;
        }();
        if (call_stop) {
          stdexec::set_stopped(std::move(self->receiver_));
        }
      }
      Channel<T, Capacity>* channel;
      Operation* self;
    };
    using EnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<EnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, OnStopRequested>;

  public:
    template <ChannelValueType U>
    Operation(Channel<T, Capacity>* self, U&& value, Receiver receiver)
      : self_(self), value_(std::forward<U>(value)), receiver_{ std::move(receiver) } {
    }

    void start() noexcept {
      bool set_value = false;
      self_->setValueImpl(
          std::move(value_), this, [this, &set_value]() { set_value = true; },
          [this]() {
            stop_callback_.emplace(stdexec::get_stop_token(stdexec::get_env(receiver_)),
                                   OnStopRequested{ self_, this });
          });
      if (set_value) {
        stdexec::set_value(std::move(receiver_));
      }
    }

    void retry() noexcept final {
      bool set_value = false;
      self_->setValueImpl(
          std::move(value_), this, [this, &set_value]() mutable { set_value = true; }, [this]() {});
      if (set_value) {
        stop_callback_.reset();
        stdexec::set_value(std::move(receiver_));
      }
    }

  private:
    Channel<T, Capacity>* self_;
    T value_;
    Receiver receiver_;
    std::optional<StopCallbackT> stop_callback_;
  };

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
  auto connect(Receiver&& receiver) && noexcept(std::is_nothrow_move_constructible_v<ReceiverT>) {
    return Operation<ReceiverT>(self, std::move(value), std::forward<Receiver>(receiver));
  }

  Channel<T, Capacity>* self;
  T value;
};
}  // namespace internal

template <ChannelValueType T, std::size_t Capacity>
template <ChannelValueType U>
auto Channel<T, Capacity>::setValue(U&& value) noexcept -> internal::SetValueSender<T, Capacity> {
  return internal::SetValueSender<T, Capacity>{ this, std::forward<U>(value) };
}

namespace internal {
template <typename T, std::size_t Capacity>
struct GetValueSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(T), stdexec::set_stopped_t()>;

  template <typename Receiver>
  class Operation : public AwaiterBase {
    struct OnStopRequested {
      void operator()() const noexcept {
        bool call_stop = [this]() {
          std::scoped_lock lock{ channel->mutex_ };
          if (channel->get_awaiters_.erase(self)) {
            return true;
          }
          return false;
        }();
        if (call_stop) {
          stdexec::set_stopped(std::move(self->receiver_));
        }
      }
      Channel<T, Capacity>* channel;
      Operation* self;
    };
    using EnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<EnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, OnStopRequested>;

  public:
    Operation(Channel<T, Capacity>* self, Receiver receiver) : self_(self), receiver_{ std::move(receiver) } {
    }

    void start() noexcept {
      std::optional<T> value;
      self_->getValueImpl(
          this, [this, &value](T&& t) { value.emplace(std::move(t)); },
          [this]() {
            stop_callback_.emplace(stdexec::get_stop_token(stdexec::get_env(receiver_)),
                                   OnStopRequested{ self_, this });
          });
      if (value.has_value()) {
        stdexec::set_value(std::move(receiver_), std::move(*value));
        return;
      }
    }

    void retry() noexcept final {
      std::optional<T> value;
      self_->getValueImpl(this, [this, &value](T&& t) mutable { value.emplace(std::move(t)); }, [this]() {});
      if (value.has_value()) {
        stop_callback_.reset();
        stdexec::set_value(std::move(receiver_), std::move(*value));
      }
    }

  private:
    Channel<T, Capacity>* self_;
    Receiver receiver_;
    std::optional<StopCallbackT> stop_callback_;
  };

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
  auto connect(Receiver&& receiver) && noexcept(std::is_nothrow_move_constructible_v<ReceiverT>) {
    return Operation<ReceiverT>(self, std::forward<Receiver>(receiver));
  }

  Channel<T, Capacity>* self;
};
}  // namespace internal

template <ChannelValueType T, std::size_t Capacity>
auto Channel<T, Capacity>::getValue() noexcept -> internal::GetValueSender<T, Capacity> {
  return internal::GetValueSender<T, Capacity>{ this };
}

}  // namespace heph::concurrency
