//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <type_traits>

#include "hephaestus/utils/exception.h"

namespace heph::containers {
namespace concepts {
template <typename T, typename U>
concept SimilarTo = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;
}  // namespace concepts

/// Queue that allows the consumer to block until new data is available, and immediately resume
/// execution when new data is written in the queue.
template <class T>
class BlockingQueue {
public:
  /// Create a queue
  /// \param max_size Max number of concurrent element in the queue. If not specified, a queue with
  /// infinite length is created.
  explicit BlockingQueue(std::optional<std::size_t> max_size) : max_size_(max_size) {
    throwExceptionIf<InvalidParameterException>(max_size_.has_value() && *max_size_ == 0,
                                                "Cannot create a queue with max size 0");
  }

  /// Attempt to enqueue the data if there is space in the queue.
  /// \note This is safe to call from multiple threads.
  /// \return true if the new data is added to the queue, false otherwise.
  template <concepts::SimilarTo<T> U>
  [[nodiscard]] auto tryPush(U&& obj) -> bool {
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      if (max_size_.has_value() && queue_.size() == *max_size_) {
        return false;
      }

      queue_.push_back(std::forward<U>(obj));
    }

    reader_signal_.notify_one();
    return true;
  }

  /// Write the data to the queue. If no space is left in the queue, the oldest element is dropped.
  /// \note This is safe to call from multiple threads.
  /// \param obj The value to push into the queue.
  /// \return If the queue was full, the element dropped to make space for the new one.
  template <concepts::SimilarTo<T> U>
  auto forcePush(U&& obj) -> std::optional<T> {
    std::optional<T> element_dropped;
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      if (max_size_.has_value() && queue_.size() == *max_size_) {
        element_dropped.emplace(std::move(queue_.front()));
        queue_.pop_front();
      }

      queue_.push_back(std::forward<U>(obj));
    }

    reader_signal_.notify_one();
    return element_dropped;
  }

  /// Write the data to the queue. If no space is left in the queue,
  /// the function blocks until either space is freed or stop is called.
  /// \note This is safe to call from multiple threads.
  template <concepts::SimilarTo<T> U>
  void waitAndPush(U&& obj) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      writer_signal_.wait(lock,
                          [this]() { return !max_size_.has_value() || queue_.size() < *max_size_ || stop_; });
      if (stop_) {
        return;
      }

      queue_.push_back(std::forward<U>(obj));
    }

    reader_signal_.notify_one();
  }

  /// Attempt to enqueue the data if there is space in the queue. Support constructing a new element
  /// in-place.
  /// \note This is safe to call from multiple threads.
  /// \return true if the new data is added to the queue, false otherwise.
  template <typename... Args>
  [[nodiscard]] auto tryEmplace(Args&&... args) -> bool {
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      if (max_size_.has_value() && queue_.size() == *max_size_) {
        return false;
      }

      queue_.emplace_back(std::forward<Args>(args)...);
    }

    reader_signal_.notify_one();
    return true;
  }

  /// Write the data to the queue. If no space is left in the queue, the oldest element is dropped.
  /// Support constructing a new element in-place.
  /// \note This is safe to call from multiple threads.
  /// \return If the queue was full, the element dropped to make space for the new one.
  template <typename... Args>
  auto forceEmplace(Args&&... args) -> std::optional<T> {
    std::optional<T> element_dropped;
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      if (max_size_.has_value() && queue_.size() == *max_size_) {
        element_dropped.emplace(std::move(queue_.front()));
        queue_.pop_front();
      }

      queue_.emplace_back(std::forward<Args>(args)...);
    }

    reader_signal_.notify_one();
    return element_dropped;
  }

  /// Write the data to the queue. If no space is left in the queue,
  /// the function blocks until either space is freed or stop is called.
  /// Support constructing a new element in-place.
  /// \note This is safe to call from multiple threads.
  template <typename... Args>
  void waitAndEmplace(Args&&... args) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      writer_signal_.wait(lock,
                          [this]() { return !max_size_.has_value() || queue_.size() < *max_size_ || stop_; });
      if (stop_) {
        return;
      }

      queue_.emplace_back(std::forward<Args>(args)...);
    }

    reader_signal_.notify_one();
  }

  /// Pop data from the queue, if data is present the function returns immediately,
  /// otherwise it blocks waiting for new data, or the stop signal is set.
  /// \note This is safe to call from multiple threads.
  /// \return The first element from the queue if the queue contains data, std::nullopt otherwise.
  [[nodiscard]] auto waitAndPop() noexcept(std::is_nothrow_move_constructible_v<T>) -> std::optional<T> {
    std::unique_lock<std::mutex> lock(mutex_);
    reader_signal_.wait(lock, [this]() { return !queue_.empty() || stop_; });
    if (stop_) {
      return {};
    }

    auto value = std::move(queue_.front());
    queue_.pop_front();
    writer_signal_.notify_one();  // Notifying a writer waiting that there is an empty space.
    return value;
  }

  /// Tries to pop data from the queue, if no data is present it returns nothing.
  /// \note This is safe to call from multiple threads.
  /// \return The first element from the queue if the queue contains data, std::nullopt otherwise.
  [[nodiscard]] auto tryPop() noexcept(std::is_nothrow_move_constructible_v<T>) -> std::optional<T> {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return {};
    }

    auto value = std::move(queue_.front());
    queue_.pop_front();
    writer_signal_.notify_one();  // Notifying a writer waiting that there is an empty space.
    return value;
  }

  /// Stop the queue, waking up all blocked consumers.
  /// \note This is safe to call from multiple threads.
  void stop() {
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    reader_signal_.notify_all();
    writer_signal_.notify_all();
  }

  [[nodiscard]] auto size() const -> std::size_t {
    const std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

  [[nodiscard]] auto empty() const -> bool {
    const std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

private:
  std::deque<T> queue_{};
  std::optional<std::size_t> max_size_;
  std::condition_variable reader_signal_;
  std::condition_variable writer_signal_;
  bool stop_{ false };
  mutable std::mutex mutex_;
};
}  // namespace heph::containers
