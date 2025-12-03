//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>
#include <boost/circular_buffer.hpp>  // IWYU pragma: keep
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::concurrency {
namespace internal {
enum class QueueAwaiterState : std::uint8_t {
  STARTING,
  ENQUEUED,
  STOPPED,
};
static_assert(std::atomic<QueueAwaiterState>::is_always_lock_free);

class AwaiterQueue;

struct AwaiterBase {
  virtual ~AwaiterBase() = default;
  void start() noexcept;
  void retry() noexcept;

  // This function is overridden by the implementation.
  // The return value determines if `set_value` was already called (true)
  // or if an enqueue happened (false).
  virtual auto startImpl() noexcept -> bool = 0;
  virtual void retryImpl() noexcept = 0;
  virtual void setStopped() noexcept = 0;

protected:
  void stopRequested();
  void finalizeStart();
  void waitForStarting() const;
  auto isStopped() const -> bool;

  struct OnStopRequested {
    void operator()() const noexcept;
    AwaiterQueue* queue;
    AwaiterBase* self;
  };

private:
  friend struct heph::containers::IntrusiveFifoQueueAccess;
  [[maybe_unused]] AwaiterBase* next_{ nullptr };
  [[maybe_unused]] AwaiterBase* prev_{ nullptr };
  std::atomic<QueueAwaiterState> state_{ QueueAwaiterState::STARTING };
};

class AwaiterQueue {
public:
  void enqueue(AwaiterBase* awaiter);

  auto erase(AwaiterBase* awaiter) -> bool;

  void retryNext();

private:
  using QueueT = containers::IntrusiveFifoQueue<AwaiterBase>;
  absl::Mutex mutex_;
  QueueT queue_ ABSL_GUARDED_BY(mutex_);
};

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
  /// Push a value into the channel. The returned sender will complete if there is space to store an
  /// element. Otherwise blocks until at least one item was consumed. Then returned sender completes with the
  /// value received from the channel.
  ///
  /// @param value The value to send via the channel
  template <ChannelValueType U>
  [[nodiscard]] auto setValue(U&& value) noexcept -> SetValueSender;

  /// Push a value into the channel
  ///
  /// Similar to \ref setValue but removes the oldest element if not enough space is available.
  template <ChannelValueType U>
  void setValueOverwrite(U&& value) noexcept;

  /// Retrieve a value stored in the channel. The returned sender will complete as soon as there is
  /// at least one item stored in the channel.
  [[nodiscard]] auto getValue() noexcept -> GetValueSender;

  /// Retrieve a value stored in the channel.
  ///
  /// Similar to \ref getValue but returns an optional which contains a value
  /// if there was an item at the time when this function was called.
  [[nodiscard]] auto tryGetValue() noexcept -> std::optional<T>;

private:
  template <typename T_, std::size_t Capacity_>
  friend struct internal::SetValueSender;
  template <typename T_, std::size_t Capacity_>
  friend struct internal::GetValueSender;

  template <ChannelValueType U>
  auto setValueImpl(U&& value, internal::AwaiterBase* set_awaiter) noexcept -> bool;
  auto getValueImpl(internal::AwaiterBase* get_awaiter) noexcept -> std::optional<T>;

private:
  absl::Mutex data_mutex_;
  // the include structure of boost::circular_buffer doesn't work correctly with IWYU
  // NOLINTNEXTLINE(misc-include-cleaner) -
  boost::circular_buffer<T> data_ ABSL_GUARDED_BY(data_mutex_){ Capacity };
  internal::AwaiterQueue set_awaiters_;
  internal::AwaiterQueue get_awaiters_;
};

template <ChannelValueType T, std::size_t Capacity>
inline auto Channel<T, Capacity>::tryGetValue() noexcept -> std::optional<T> {
  return getValueImpl(nullptr);
}

template <ChannelValueType T, std::size_t Capacity>
inline auto Channel<T, Capacity>::getValueImpl(internal::AwaiterBase* get_awaiter) noexcept
    -> std::optional<T> {
  std::optional<T> res;
  {
    absl::MutexLock lock{ &data_mutex_ };
    if (data_.empty()) {
      get_awaiters_.enqueue(get_awaiter);
      return res;
    }
    res.emplace(std::move(data_.front()));
    data_.pop_front();
  }
  set_awaiters_.retryNext();
  return res;
}

namespace internal {
template <typename T, std::size_t Capacity>
struct GetValueSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(T), stdexec::set_stopped_t()>;

  template <typename Receiver>
  class Operation : public AwaiterBase {
    using OnStopRequested = AwaiterBase::OnStopRequested;
    using EnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<EnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, OnStopRequested>;

  public:
    Operation(Channel<T, Capacity>* self, Receiver receiver)
      : channel_(self), receiver_{ std::move(receiver) } {
    }

    auto startImpl() noexcept -> bool final {
      auto value = channel_->getValueImpl(this);
      if (value.has_value()) {
        stdexec::set_value(std::move(receiver_), std::move(*value));
        return true;
      }
      stop_callback_.emplace(stdexec::get_stop_token(stdexec::get_env(receiver_)),
                             OnStopRequested{ &channel_->get_awaiters_, this });
      return false;
    }

    void retryImpl() noexcept final {
      auto value = channel_->getValueImpl(this);
      if (value.has_value()) {
        stop_callback_.reset();
        stdexec::set_value(std::move(receiver_), std::move(*value));
      } else {
        // Account for potentially concurrent stop signals arriving which we might have missed
        if (stdexec::get_stop_token(stdexec::get_env(receiver_)).stop_requested()) {
          // If we failed, we got enqueued. We need to revert this to ensure proper lifetime
          if (channel_->get_awaiters_.erase(this)) {
            setStopped();
          }
        }
      }
    }

  private:
    void setStopped() noexcept final {
      stdexec::set_stopped(std::move(receiver_));
    }

  private:
    std::atomic<QueueAwaiterState> state_{ QueueAwaiterState::STARTING };
    Channel<T, Capacity>* channel_;
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

template <ChannelValueType T, std::size_t Capacity>
template <ChannelValueType U>
inline void Channel<T, Capacity>::setValueOverwrite(U&& value) noexcept {
  {
    absl::MutexLock lock{ &data_mutex_ };
    if (data_.full()) {
      data_.pop_front();
    }
    data_.push_back(std::forward<U>(value));
  }
  get_awaiters_.retryNext();
}

template <ChannelValueType T, std::size_t Capacity>
template <ChannelValueType U>
inline auto Channel<T, Capacity>::setValueImpl(U&& value, internal::AwaiterBase* set_awaiter) noexcept
    -> bool {
  {
    absl::MutexLock lock{ &data_mutex_ };
    if (data_.full()) {
      set_awaiters_.enqueue(set_awaiter);
      return false;
    }
    data_.push_back(std::forward<U>(value));
  }
  get_awaiters_.retryNext();
  return true;
}

namespace internal {
template <typename T, std::size_t Capacity>
struct SetValueSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t()>;

  template <typename Receiver>
  class Operation : public AwaiterBase {
    using OnStopRequested = AwaiterBase::OnStopRequested;
    using EnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<EnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, OnStopRequested>;

  public:
    template <ChannelValueType U>
    Operation(Channel<T, Capacity>* self, U&& value, Receiver receiver)
      : channel_(self), value_(std::forward<U>(value)), receiver_{ std::move(receiver) } {
    }

    auto startImpl() noexcept -> bool final {
      auto set_value = channel_->setValueImpl(std::move(value_), this);
      if (set_value) {
        stdexec::set_value(std::move(receiver_));
        return true;
      }
      stop_callback_.emplace(stdexec::get_stop_token(stdexec::get_env(receiver_)),
                             OnStopRequested{ &channel_->set_awaiters_, this });
      return false;
    }

    void retryImpl() noexcept final {
      auto success = channel_->setValueImpl(std::move(value_), this);
      if (success) {
        stop_callback_.reset();
        stdexec::set_value(std::move(receiver_));
      } else {
        // Account for potentially concurrent stop signals arriving which we might have missed
        if (stdexec::get_stop_token(stdexec::get_env(receiver_)).stop_requested()) {
          // If we failed, we got enqueued. We need to revert this to ensure proper lifetime
          if (channel_->set_awaiters_.erase(this)) {
            setStopped();
          }
        }
      }
    }

  private:
    void setStopped() noexcept final {
      stdexec::set_stopped(std::move(receiver_));
    }

  private:
    Channel<T, Capacity>* channel_;
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
}  // namespace heph::concurrency
