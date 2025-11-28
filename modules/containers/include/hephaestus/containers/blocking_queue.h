//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>

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
  /// \param max_size Max number of concurrent element in the queue.
  explicit BlockingQueue(std::size_t max_size) : max_size_(max_size) {
  }

  /// Attempt to enqueue the data if there is space in the queue.
  /// \note This is safe to call from multiple threads.
  /// \return true if the new data is added to the queue, false otherwise.
  template <concepts::SimilarTo<T> U>
  [[nodiscard]] auto tryPush(U&& obj) -> bool {
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      if (stop_) {
        return false;
      }

      if (queue_.size() == max_size_) {
        return false;
      }

      queue_.push_back(std::forward<U>(obj));
    }

    if (waiting_readers_ > 0) {
      reader_signal_.notify_one();
    }
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
      if (stop_) {
        return T{ std::forward<U>(obj) };  // We discard the input object if the queue is
                                           // stopped.
      }

      if (queue_.size() == max_size_) {
        element_dropped.emplace(std::move(queue_.front()));
        queue_.pop_front();
      }

      queue_.push_back(std::forward<U>(obj));
    }

    if (waiting_readers_ > 0) {
      reader_signal_.notify_one();
    }
    return element_dropped;
  }

  /// Write the data to the queue. If no space is left in the queue,
  /// the function blocks until either space is freed or stop is called.
  /// \note This is safe to call from multiple threads.
  template <concepts::SimilarTo<T> U>
  void waitAndPush(U&& obj) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (stop_) {
        return;
      }

      ++waiting_writers_;
      writer_signal_.wait(lock, [this]() { return queue_.size() < max_size_ || stop_; });
      if (stop_) {
        --waiting_writers_;
        return;
      }
      --waiting_writers_;

      queue_.push_back(std::forward<U>(obj));
    }

    if (waiting_readers_ > 0) {
      reader_signal_.notify_one();
    }
  }

  /// Attempt to enqueue the data if there is space in the queue. Support constructing a new element
  /// in-place.
  /// \note This is safe to call from multiple threads.
  /// \return true if the new data is added to the queue, false otherwise.
  template <typename... Args>
  [[nodiscard]] auto tryEmplace(Args&&... args) -> bool {
    {
      const std::unique_lock<std::mutex> lock(mutex_);
      if (stop_) {
        return false;
      }

      if (queue_.size() == max_size_) {
        return false;
      }

      queue_.emplace_back(std::forward<Args>(args)...);
    }

    if (waiting_readers_ > 0) {
      reader_signal_.notify_one();
    }
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
      if (stop_) {
        return T{ std::forward<Args>(args)... };
      }

      const std::unique_lock<std::mutex> lock(mutex_);
      if (queue_.size() == max_size_) {
        element_dropped.emplace(std::move(queue_.front()));
        queue_.pop_front();
      }

      queue_.emplace_back(std::forward<Args>(args)...);
    }

    if (waiting_readers_ > 0) {
      reader_signal_.notify_one();
    }
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
      if (stop_) {
        return;
      }

      ++waiting_writers_;
      writer_signal_.wait(lock, [this]() { return queue_.size() < max_size_ || stop_; });
      if (stop_) {
        --waiting_writers_;
        return;
      }
      --waiting_writers_;

      queue_.emplace_back(std::forward<Args>(args)...);
    }

    if (waiting_readers_ > 0) {
      reader_signal_.notify_one();
    }
  }

  /// Pop data from the queue, if data is present the function returns immediately,
  /// otherwise it blocks waiting for new data, or the stop signal is set.
  /// \note This is safe to call from multiple threads.
  /// \return The first element from the queue if the queue contains data, std::nullopt otherwise.
  [[nodiscard]] auto waitAndPop() noexcept(std::is_nothrow_move_constructible_v<T>) -> std::optional<T> {
    std::unique_lock<std::mutex> lock(mutex_);
    if (stop_) {
      return {};
    }

    ++waiting_readers_;
    reader_signal_.wait(lock, [this]() { return !queue_.empty() || stop_; });
    if (stop_) {
      --waiting_readers_;
      return {};
    }
    --waiting_readers_;

    auto value = std::move(queue_.front());
    queue_.pop_front();
    if (waiting_writers_ > 0) {
      writer_signal_.notify_one();  // Notifying a writer waiting that there is an empty space.
    }
    return value;
  }

  [[nodiscard]] auto waitAndPopAll() noexcept(std::is_nothrow_move_constructible_v<T>) -> std::deque<T> {
    std::unique_lock<std::mutex> lock(mutex_);
    if (stop_) {
      return {};
    }

    ++waiting_readers_;
    reader_signal_.wait(lock, [this]() { return !queue_.empty() || stop_; });
    --waiting_readers_;
    if (stop_) {
      return {};
    }

    std::deque<T> res;
    std::swap(res, queue_);
    if (waiting_writers_ > 0) {
      writer_signal_.notify_one();  // Notifying a writer waiting that there is an empty space.
    }
    return res;
  }

  /// Tries to pop data from the queue, if no data is present it returns nothing.
  /// \note This is safe to call from multiple threads.
  /// \return The first element from the queue if the queue contains data, std::nullopt otherwise.
  [[nodiscard]] auto tryPop() noexcept(std::is_nothrow_move_constructible_v<T>) -> std::optional<T> {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (stop_ || queue_.empty()) {
      return {};
    }

    auto value = std::move(queue_.front());
    queue_.pop_front();
    if (waiting_writers_ > 0) {
      writer_signal_.notify_one();  // Notifying a writer waiting that there is an empty space.
    }
    return value;
  }

  /// Stop the queue, waking up all blocked consumers.
  /// \note This is safe to call from multiple threads.
  void stop() {
    const std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    reader_signal_.notify_all();
    writer_signal_.notify_all();
  }

  void restart() {
    stop();

    // Wait until noone is stuck in the queue.
    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
      // We can do this with a polling loop, because we just need to wait for the condition variable to wake
      // the code for it to terminate.
      // It is guaranteed that no new readers or writers will be added to the queue as it is stopped.
      if (waiting_readers_ == 0 && waiting_writers_ == 0) {
        break;
      }

      lock.unlock();
      std::this_thread::yield();
      lock.lock();
    }

    queue_.clear();
    stop_ = false;
  }

  [[nodiscard]] auto size() const -> std::size_t {
    const std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

  [[nodiscard]] auto empty() const -> bool {
    const std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  /// Wait until the queue is empty.
  [[nodiscard]]
  auto waitForEmpty() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty() || stop_) {
      return;
    }
    reader_signal_.wait(lock, [this]() { return !queue_.empty() || stop_; });
  }

private:
  std::deque<T> queue_{};
  std::size_t max_size_;
  std::condition_variable reader_signal_;
  std::condition_variable writer_signal_;
  mutable std::mutex mutex_;
  std::size_t waiting_readers_{ 0 };
  std::size_t waiting_writers_{ 0 };
  bool stop_{ false };
};
}  // namespace heph::containers
