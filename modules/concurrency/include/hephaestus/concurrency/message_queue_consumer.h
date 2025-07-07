//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <functional>
#include <future>
#include <optional>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/containers/blocking_queue.h"

namespace heph::concurrency {

/// MessageQueueConsumer creates a thread that wait for messages to be pushed in the queue and call the user
/// provided callback on them.
template <typename T>
class MessageQueueConsumer {
public:
  using Callback = std::function<void(T&&)>;
  [[nodiscard]] MessageQueueConsumer(Callback&& callback, std::optional<std::size_t> max_queue_size);
  ~MessageQueueConsumer() = default;
  MessageQueueConsumer(const MessageQueueConsumer&) = delete;
  MessageQueueConsumer(MessageQueueConsumer&&) = delete;
  auto operator=(const MessageQueueConsumer&) -> MessageQueueConsumer& = delete;
  auto operator=(MessageQueueConsumer&&) -> MessageQueueConsumer& = delete;

  void start();

  /// @brief Stop the message consumer, emptying the queue and stopping the processing.
  /// @return future that waits on the queue to be emptied
  [[nodiscard]] auto stop() -> std::future<void>;

  /// Return the queue to allow to push messages to process.
  /// This is simpler than having to mask all the different type of push methods of the queue.
  /// The downside is that it is possible to consume messages from the outside without calling the callback.
  [[nodiscard]] auto queue() -> containers::BlockingQueue<T>&;

private:
  void consume();

private:
  Callback callback_;
  containers::BlockingQueue<T> message_queue_;

  Spinner spinner_;
};

template <typename T>
MessageQueueConsumer<T>::MessageQueueConsumer(Callback&& callback, std::optional<std::size_t> max_queue_size)
  : callback_(std::move(callback))
  , message_queue_(max_queue_size)
  , spinner_(Spinner::createNeverStoppingCallback(std::move([this] { consume(); }))) {
}

template <typename T>
void MessageQueueConsumer<T>::start() {
  spinner_.start();
}

template <typename T>
auto MessageQueueConsumer<T>::stop() -> std::future<void> {
  message_queue_.stop();
  spinner_.stop();

  return std::async(std::launch::async, [this]() {
    while (!message_queue_.empty()) {
      auto message = message_queue_.tryPop();
      if (!message.has_value()) {
        return;
      }

      callback_(std::move(message.value()));
    }
  });
}

template <typename T>
auto MessageQueueConsumer<T>::queue() -> containers::BlockingQueue<T>& {
  return message_queue_;
}

template <typename T>
void MessageQueueConsumer<T>::consume() {
  auto message = message_queue_.waitAndPop();
  if (!message.has_value()) {
    return;
  }

  callback_(std::move(message.value()));
}

}  // namespace heph::concurrency
