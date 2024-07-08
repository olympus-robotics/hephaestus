//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <thread>

#include "hephaestus/containers/blocking_queue.h"

namespace heph::concurrency {

/// MessageQueueConsumer creates a thread that wait for messages to be pushed in the queue and call the user
/// provided callback on them.
template <typename T>
class MessageQueueConsumer {
public:
  using Callback = std::function<void(const T&)>;
  [[nodiscard]] MessageQueueConsumer(Callback&& callback, std::optional<std::size_t> max_queue_size);
  ~MessageQueueConsumer();
  MessageQueueConsumer(const MessageQueueConsumer&) = delete;
  MessageQueueConsumer(MessageQueueConsumer&&) = delete;
  auto operator=(const MessageQueueConsumer&) -> MessageQueueConsumer& = delete;
  auto operator=(MessageQueueConsumer&&) -> MessageQueueConsumer& = delete;

  /// Return the queue to allow to push messages to process.
  /// This is simpler than having to mask all the different type of push methods of the queue.
  [[nodiscard]] auto queue() -> containers::BlockingQueue<T>&;

private:
  void spin();

private:
  Callback callback_;
  containers::BlockingQueue<T> message_queue_;

  std::thread callback_thread_;
};

template <typename T>
MessageQueueConsumer<T>::MessageQueueConsumer(Callback&& callback, std::optional<std::size_t> max_queue_size)
  : callback_(std::move(callback)), message_queue_(max_queue_size), callback_thread_([this] { spin(); }){};

template <typename T>
MessageQueueConsumer<T>::~MessageQueueConsumer() {
  message_queue_.stop();
  callback_thread_.join();
}

template <typename T>
auto MessageQueueConsumer<T>::queue() -> containers::BlockingQueue<T>& {
  return message_queue_;
}

template <typename T>
void MessageQueueConsumer<T>::spin() {
  // TODO: set thread name
  while (true) {
    auto message = message_queue_.waitAndPop();
    if (!message.has_value()) {
      break;
    }

    callback_(message.value());
  }
}

}  // namespace heph::concurrency
