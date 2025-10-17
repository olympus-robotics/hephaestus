//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

#include <absl/synchronization/mutex.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/internal/circular_buffer.h"
#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::concurrency {
namespace internal {
struct AwaiterBase {
  virtual ~AwaiterBase() = default;
  virtual void restart() noexcept = 0;
  virtual void stop() noexcept = 0;

private:
  friend struct heph::containers::IntrusiveFifoQueueAccess;
  [[maybe_unused]] AwaiterBase* next_{ nullptr };
  [[maybe_unused]] AwaiterBase* prev_{ nullptr };
};

struct StopCallback {
  AwaiterBase* self;
  void operator()() const noexcept {
    self->stop();
  }
};

using AwaiterQueueT = containers::IntrusiveFifoQueue<AwaiterBase>;

template <typename T, std::size_t Capacity>
struct GetValueSender;
template <typename T, std::size_t Capacity>
struct SetValueSender;
}  // namespace internal

/// Asynchronous communication channel
///
/// The member functions \ref getValue and \ref setValue return senders.
/// For waiting on them, use `stdexec::sync_wait`.
///
/// @tparam T The type of the values to store
/// @tparam Capacity Maximum number of elements the Channel can store
///
/// Example:
/// @code
/// Channel<int, 10> c;
///
/// // Using Sender/Receiver
/// // Producer:
/// c.setValue(42) | stdexec::then([](){ fmt::println("Value has been succesully set!"); });
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
template <typename T, std::size_t Capacity>
class Channel {
  using GetValueSender = internal::GetValueSender<T, Capacity>;
  using SetValueSender = internal::SetValueSender<T, Capacity>;

public:
  /// Retrieve a value stored in the channel. The returned sender will complete as soon as there is
  /// at least one item stored in the channel.
  ///
  /// @param t The value to send via the channel
  [[nodiscard]] auto setValue(T t) -> SetValueSender;
  /// Push a value into the channel. The returned sender will complete if there is space to store an
  /// element. Otherwise blocks until at least one item was consumed. Then returned sender completes with the
  /// value received from the channel.
  [[nodiscard]] auto getValue() -> GetValueSender;

  [[nodiscard]] auto size() const -> std::size_t {
    absl::MutexLock l{ &mutex_ };
    return data_.size();
  }

private:
  template <typename T_, std::size_t Capacity_>
  friend struct internal::SetValueSender;
  template <typename T_, std::size_t Capacity_>
  friend struct internal::GetValueSender;

  auto setValueImpl(T&& t, internal::AwaiterBase* set_awaiter) -> bool {
    internal::AwaiterBase* get_awaiter{ nullptr };
    {
      absl::MutexLock l{ &mutex_ };
      if (!data_.push(std::move(t))) {
        set_awaiters_.enqueue(set_awaiter);
        return false;
      }

      get_awaiter = get_awaiters_.dequeue();
    }
    if (get_awaiter != nullptr) {
      get_awaiter->restart();
    }
    return true;
  }

  auto getValueImpl(internal::AwaiterBase* get_awaiter) -> std::optional<T> {
    internal::AwaiterBase* set_awaiter{ nullptr };
    std::optional<T> res;
    {
      absl::MutexLock l{ &mutex_ };
      res = data_.pop();
      if (!res.has_value()) {
        get_awaiters_.enqueue(get_awaiter);
        return std::nullopt;
      }

      set_awaiter = set_awaiters_.dequeue();
    }
    if (set_awaiter != nullptr) {
      set_awaiter->restart();
    }

    return res;
  }

private:
  mutable absl::Mutex mutex_;
  internal::CircularBuffer<T, Capacity> data_;
  internal::AwaiterQueueT set_awaiters_;
  internal::AwaiterQueueT get_awaiters_;
};

namespace internal {
template <typename T, std::size_t Capacity>
struct SetValueSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t(),
                                     stdexec::set_error_t(std::exception_ptr)>;

  template <typename Receiver>
  class Operation : public AwaiterBase {
    using ReceiverEnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<ReceiverEnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, internal::StopCallback>;

  public:
    Operation(Channel<T, Capacity>* self, T value, Receiver receiver)
      : self_(self), value_(std::move(value)), receiver_(std::move(receiver)) {
    }

    void start() noexcept {
      auto stop_token = stdexec::get_stop_token(stdexec::get_env(receiver_));
      if (stop_token.stop_requested()) {
        stdexec::set_stopped(std::move(receiver_));
      }
      if (!self_->setValueImpl(std::move(value_), this)) {
        stop_callback_.emplace(stop_token, StopCallback{ this });
        return;
      }
      stdexec::set_value(std::move(receiver_));
    }

    void restart() noexcept final {
      if (self_->setValueImpl(std::move(value_), this)) {
        stdexec::set_value(std::move(receiver_));
      }
    }

    void stop() noexcept final {
      {
        absl::MutexLock l{ &self_->mutex_ };
        self_->set_awaiters_.erase(this);
      }
      stdexec::set_stopped(std::move(receiver_));
    }

  private:
    Channel<T, Capacity>* self_;
    T value_;
    Receiver receiver_;
    std::optional<StopCallbackT> stop_callback_;
  };

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
  auto connect(Receiver&& receiver) && {
    return Operation<ReceiverT>(self, std::move(value), std::forward<Receiver>(receiver));
  }

  Channel<T, Capacity>* self;
  T value;
};
}  // namespace internal

template <typename T, std::size_t Capacity>
auto Channel<T, Capacity>::setValue(T t) -> internal::SetValueSender<T, Capacity> {
  return internal::SetValueSender<T, Capacity>{ this, std::move(t) };
}

namespace internal {
template <typename T, std::size_t Capacity>
struct GetValueSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(T), stdexec::set_stopped_t(),
                                     stdexec::set_error_t(std::exception_ptr)>;

  template <typename Receiver>
  class Operation : public AwaiterBase {
    using ReceiverEnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<ReceiverEnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, internal::StopCallback>;

  public:
    Operation(Channel<T, Capacity>* self, Receiver receiver) : self_(self), receiver_(std::move(receiver)) {
    }

    void start() noexcept {
      auto stop_token = stdexec::get_stop_token(stdexec::get_env(receiver_));
      if (stop_token.stop_requested()) {
        stdexec::set_stopped(std::move(receiver_));
      }
      auto res = self_->getValueImpl(this);
      if (res == std::nullopt) {
        stop_callback_.emplace(stop_token, StopCallback{ this });
        return;
      }
      stdexec::set_value(std::move(receiver_), std::move(*res));
    }

    void restart() noexcept final {
      auto res = self_->getValueImpl(this);
      if (res != std::nullopt) {
        stop_callback_.reset();
        stdexec::set_value(std::move(receiver_), std::move(*res));
      }
    }

    void stop() noexcept final {
      {
        absl::MutexLock l{ &self_->mutex_ };
        stop_callback_.reset();
        self_->get_awaiters_.erase(this);
      }
      stdexec::set_stopped(std::move(receiver_));
    }

  private:
    Channel<T, Capacity>* self_;
    Receiver receiver_;
    std::optional<StopCallbackT> stop_callback_;
  };

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
  auto connect(Receiver&& receiver) && {
    return Operation<ReceiverT>(self, std::forward<Receiver>(receiver));
  }

  Channel<T, Capacity>* self;
};
}  // namespace internal

template <typename T, std::size_t Capacity>
auto Channel<T, Capacity>::getValue() -> internal::GetValueSender<T, Capacity> {
  return internal::GetValueSender<T, Capacity>{ this };
}

}  // namespace heph::concurrency
