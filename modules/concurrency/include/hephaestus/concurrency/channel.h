//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/internal/circular_buffer.h"
#include "hephaestus/concurrency/stoppable_operation_state.h"
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
    absl::MutexLock lock{ &mutex_ };
    return data_.size();
  }

  void setValueOverwrite(T t) {
    internal::AwaiterBase* get_awaiter{ nullptr };
    {
      absl::MutexLock lock{ &mutex_ };
      while (!data_.push(std::move(t))) {
        data_.pop();
      }

      get_awaiter = get_awaiters_.dequeue();
    }
    if (get_awaiter != nullptr) {
      get_awaiter->restart();
    }
  }

private:
  template <typename T_, std::size_t Capacity_>
  friend struct internal::SetValueSender;
  template <typename T_, std::size_t Capacity_>
  friend struct internal::GetValueSender;

  auto setValueImpl(T&& t, internal::AwaiterBase* set_awaiter) -> bool {
    internal::AwaiterBase* get_awaiter{ nullptr };
    {
      absl::MutexLock lock{ &mutex_ };
      if (!data_.push(std::move(t))) {
        set_awaiters_.enqueue(set_awaiter);
        assert(get_awaiters_.size() == 0);
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
      absl::MutexLock lock{ &mutex_ };
      res = data_.pop();
      if (!res.has_value()) {
        get_awaiters_.enqueue(get_awaiter);
        assert(set_awaiters_.size() == 0);
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
  internal::CircularBuffer<T, Capacity> data_ ABSL_GUARDED_BY(mutex_);
  internal::AwaiterQueueT set_awaiters_ ABSL_GUARDED_BY(mutex_);
  internal::AwaiterQueueT get_awaiters_ ABSL_GUARDED_BY(mutex_);
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
  public:
    Operation(Channel<T, Capacity>* self, T value, Receiver receiver)
      : self_(self), value_(std::move(value)), operation_state_{ std::move(receiver), [this]() { stop(); } } {
    }

    void start() noexcept {
      auto start_transition = operation_state_.start();
      setValue();
    }

    void restart() noexcept final {
      auto start_transition = operation_state_.restart();
      if (start_transition.has_value()) {
        stop();
        setValue();
      }
    }

    void stop() noexcept final {
      {
        absl::MutexLock lock{ &self_->mutex_ };
        self_->set_awaiters_.erase(this);
      }
    }

  private:
    void setValue() {
      if (self_->setValueImpl(std::move(value_), this)) {
        operation_state_.setValue();
      }
    }

  private:
    Channel<T, Capacity>* self_;
    T value_;
    StoppableOperationState<Receiver> operation_state_;
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
  public:
    Operation(Channel<T, Capacity>* self, Receiver receiver)
      : self_(self), operation_state_{ std::move(receiver), [this]() { stop(); } } {
    }

    void start() noexcept {
      auto start_transition = operation_state_.start();
      getValue();
    }

    void restart() noexcept final {
      auto start_transition = operation_state_.restart();
      if (start_transition.has_value()) {
        stop();
        getValue();
      }
    }

    void stop() noexcept final {
      {
        absl::MutexLock lock{ &self_->mutex_ };
        self_->get_awaiters_.erase(this);
      }
    }

  private:
    auto getValue() {
      auto value = self_->getValueImpl(this);
      if (value.has_value()) {
        operation_state_.setValue(std::move(*value));
      }
    }

  private:
    Channel<T, Capacity>* self_;
    StoppableOperationState<Receiver, T> operation_state_;
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
