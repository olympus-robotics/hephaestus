//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

namespace heph::containers {
namespace internal {
template <std::size_t Capacity>
class UnsynchronizedIndices {
public:
  [[nodiscard]] auto empty() const {
    return size_ == 0;
  }

  [[nodiscard]] auto full() const {
    return size_ == Capacity;
  }

  [[nodiscard]] auto size() const -> std::size_t {
    return size_;
  }

  template <typename T, typename U>
  auto push(std::array<T, Capacity>& data, U&& t) -> bool {
    if (full()) {
      return false;
    }

    data[writeIndex()] = std::forward<U>(t);
    incrementWriteIndex();

    return true;
  }

  template <typename T>
  auto pop(std::array<T, Capacity>& data) -> std::optional<T> {
    std::optional<T> res;
    if (empty()) {
      return res;
    }

    res.emplace(std::move(data[readIndex()]));
    incrementReadIndex();

    return res;
  }

private:
  [[nodiscard]] auto writeIndex() const -> std::size_t {
    return write_index_;
  }

  auto incrementWriteIndex() -> void {
    write_index_ = (write_index_ + 1) % Capacity;
    ++size_;
  }

  [[nodiscard]] auto readIndex() const -> std::size_t {
    return read_index_;
  }

  auto incrementReadIndex() -> void {
    read_index_ = (read_index_ + 1) % Capacity;
    --size_;
  }

private:
  std::size_t read_index_{ 0 };
  std::size_t write_index_{ 0 };
  std::size_t size_{ 0 };
};

template <std::size_t Capacity>
class SpscIndices {
public:
  [[nodiscard]] auto empty() const {
    return read_index_.load(std::memory_order_acquire) == write_index_.load(std::memory_order_acquire);
  }

  [[nodiscard]] auto full() const {
    return size() == Capacity;
  }

  [[nodiscard]] auto size() const -> std::size_t {
    return write_index_.load(std::memory_order_acquire) - read_index_.load(std::memory_order_acquire);
  }

  template <typename T, typename U>
  auto push(std::array<T, Capacity>& data, U&& t) -> bool {
    std::size_t write_index{ 0 };

    write_index = write_index_.load(std::memory_order_relaxed);

    // Check if our cached read index is still within range
    if (cached_read_index_ + Capacity - write_index < 1) {
      cached_read_index_ = read_index_.load(std::memory_order_acquire);
      // Check again after loading the real read index
      if (cached_read_index_ + Capacity - write_index < 1) {
        return false;
      }
    }
    data[(write_index + 1) % Capacity] = std::forward<U>(t);
    write_index_.store(write_index + 1, std::memory_order_release);
    return true;
  }

  template <typename T>
  auto pop(std::array<T, Capacity>& data) -> std::optional<T> {
    std::optional<T> res;
    std::size_t read_index{ 0 };

    read_index = read_index_.load(std::memory_order_relaxed);
    if (cached_write_index_ - read_index < 1) {
      cached_write_index_ = write_index_.load(std::memory_order_acquire);
      // Check again after loading the real read index
      if (cached_write_index_ - read_index < 1) {
        return res;
      }
    }
    res.emplace(std::move(data[(read_index + 1) % Capacity]));
    read_index_.store(read_index + 1, std::memory_order_release);
    return res;
  }

private:
  // Align atomics to a cache line to avoid false sharing. The cache variables are used
  // to reduce cache coherency traffic.
  static constexpr auto CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
  alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> write_index_{ 0 };
  alignas(CACHE_LINE_SIZE) std::size_t cached_read_index_{ 0 };
  alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> read_index_{ 0 };
  alignas(CACHE_LINE_SIZE) std::size_t cached_write_index_{ 0 };
};

}  // namespace internal

enum class FixedCircularBufferMode : std::uint8_t {
  UNSYNCHRONIZED,
  SPSC,
};

/// Implementation of a fixed capacity circular buffer
///
/// \tparam T the value type of the elements
/// \tparam Capacity Maximum capacity. Should be a power of two for efficiency reasons.
/// \tparam Mode can be used to switch from UNSYNCHRONIZED (default, NOT thread safe) to
///         a single producer and single consumer (SBSC)
///
/// The SBSC version uses the Implementation presented here:
/// https://inria.hal.science/hal-00862450/document
template <typename T, std::size_t Capacity,
          FixedCircularBufferMode Mode = FixedCircularBufferMode::UNSYNCHRONIZED>
class FixedCircularBuffer {
public:
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity should be a power of two.");

  FixedCircularBuffer() = default;
  ~FixedCircularBuffer() = default;
  FixedCircularBuffer(FixedCircularBuffer&& other) noexcept;
  auto operator=(FixedCircularBuffer&& other) noexcept -> FixedCircularBuffer&;

  FixedCircularBuffer(const FixedCircularBuffer& other) = delete;
  auto operator=(const FixedCircularBuffer& other) -> FixedCircularBuffer& = delete;

  [[nodiscard]] auto empty() const -> bool;
  [[nodiscard]] auto full() const -> bool;
  [[nodiscard]] auto size() const -> std::size_t;

  /// Pushes \param u onto the buffer only if the buffer is not full.
  ///
  /// \return true if space was available, false otherwise
  template <std::convertible_to<T> U>
  auto push(U&& u) -> bool;

  /// Pushes \param u onto the buffer.
  ///
  /// If the buffer was full, pops the first element.
  template <std::convertible_to<T> U>
  auto pushForce(U&& u) -> void;

  /// Returns the first item in the buffer
  ///
  /// \return std::nullopt if buffer is empty, an optional containing the value of the first element otherwise
  [[nodiscard]] auto pop() -> std::optional<T>;

private:
  using IndicesT =
      std::conditional_t<Mode == FixedCircularBufferMode::UNSYNCHRONIZED,
                         internal::UnsynchronizedIndices<Capacity>, internal::SpscIndices<Capacity>>;

  std::array<T, Capacity> data_{};
  IndicesT indices_;
};

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
inline FixedCircularBuffer<T, Capacity, Mode>::FixedCircularBuffer(FixedCircularBuffer&& other) noexcept
  : data_(std::move(data_)), indices_(std::exchange(other.indices_, IndicesT{})) {
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
inline auto FixedCircularBuffer<T, Capacity, Mode>::operator=(FixedCircularBuffer&& other) noexcept
    -> FixedCircularBuffer& {
  if (this != &other) {
    data_ = std::move(data_);
    indices_ = std::exchange(other.indices_, IndicesT{});
  }

  return *this;
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
inline auto FixedCircularBuffer<T, Capacity, Mode>::empty() const -> bool {
  return indices_.empty();
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
inline auto FixedCircularBuffer<T, Capacity, Mode>::full() const -> bool {
  return indices_.full();
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
inline auto FixedCircularBuffer<T, Capacity, Mode>::size() const -> std::size_t {
  return indices_.size();
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity, Mode>::push(U&& u) -> bool {
  return indices_.push(data_, std::forward<U>(u));
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity, Mode>::pushForce(U&& u) -> void {
  static_assert(Mode != FixedCircularBufferMode::SPSC, "force push is not supported for SPSC");
  while (!push(std::forward<U>(u))) {
    [[maybe_unused]] auto popped_element = pop();
    assert(popped_element.has_value());
  }
}

template <typename T, std::size_t Capacity, FixedCircularBufferMode Mode>
inline auto FixedCircularBuffer<T, Capacity, Mode>::pop() -> std::optional<T> {
  return indices_.pop(data_);
}
}  // namespace heph::containers
