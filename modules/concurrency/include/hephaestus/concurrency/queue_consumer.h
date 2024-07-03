//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <thread>

#include "hephaestus/containers/blocking_queue.h"

namespace heph::concurrency {

/// QueueConsumer creates a thread that wait for messages to be pushed in the queue and call the user
/// provided callback on them.
template <typename T>
class QueueConsumer {
public:
  using Callback = std::function<void(const T&)>;
  QueueConsumer(Callback&& callback, std::optional<std::size_t> max_queue_size);
  ~QueueConsumer();
  QueueConsumer(const QueueConsumer&) = delete;
  QueueConsumer(QueueConsumer&&) = delete;
  auto operator=(const QueueConsumer&) -> QueueConsumer& = delete;
  auto operator=(QueueConsumer&&) -> QueueConsumer& = delete;

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
QueueConsumer<T>::QueueConsumer(Callback&& callback, std::optional<std::size_t> max_queue_size)
  : callback_(std::move(callback)), message_queue_(max_queue_size), callback_thread_([this] { spin(); }){};

template <typename T>
QueueConsumer<T>::~QueueConsumer() {
  message_queue_.stop();
  callback_thread_.join();
}

template <typename T>
auto QueueConsumer<T>::queue() -> containers::BlockingQueue<T>& {
  return message_queue_;
}

template <typename T>
void QueueConsumer<T>::spin() {
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
