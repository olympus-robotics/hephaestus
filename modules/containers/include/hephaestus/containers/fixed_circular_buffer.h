//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace heph::containers {
/// Implementation of a fixed capacity circular buffer
///
/// \tparam T the value type of the elements. T needs to be default constructible and move operations shall
/// not throw
/// \tparam Capacity Maximum capacity. Should be a power of two for efficiency reasons.
///
/// \note This class is not thread safe. If you need to push/pop from differeent threads, protect it with a
/// lock!
template <typename T, std::size_t Capacity>
class FixedCircularBuffer {
public:
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity should be a power of two.");
  static_assert(std::is_default_constructible_v<T>, "T needs to be default constructible");
  static_assert(std::is_nothrow_move_constructible_v<T>, "T needs to be noexcept move constructible");
  static_assert(std::is_nothrow_move_assignable_v<T>, "T needs to be noexcept move assignable");

  FixedCircularBuffer() = default;
  ~FixedCircularBuffer() = default;
  FixedCircularBuffer(FixedCircularBuffer&& other) noexcept;
  auto operator=(FixedCircularBuffer&& other) noexcept -> FixedCircularBuffer&;

  FixedCircularBuffer(const FixedCircularBuffer& other) = default;
  auto operator=(const FixedCircularBuffer& other) -> FixedCircularBuffer& = default;

  [[nodiscard]] auto empty() const -> bool;
  [[nodiscard]] auto full() const -> bool;
  [[nodiscard]] auto size() const -> std::size_t;

  /// Pushes \param u onto the buffer only if the buffer is not full.
  ///
  /// \return true if space was available, false otherwise
  template <std::convertible_to<T> U>
  auto tryPush(U&& value) -> bool;

  /// Pushes \param u onto the buffer.
  ///
  /// If the buffer was full, pops the first element.
  template <std::convertible_to<T> U>
  auto forcePush(U&& value) -> void;

  /// Returns the first item in the buffer
  ///
  /// \return std::nullopt if buffer is empty, an optional containing the value of the first element otherwise
  [[nodiscard]] auto tryPop() -> std::optional<T>;

private:
  std::array<T, Capacity> data_{};
  std::size_t write_index_{ 0 };
  std::size_t read_index_{ 0 };

  static constexpr std::size_t INDEX_MASK = Capacity - 1;
};

template <typename T, std::size_t Capacity>
inline FixedCircularBuffer<T, Capacity>::FixedCircularBuffer(FixedCircularBuffer&& other) noexcept
  : data_(std::move(data_))
  , write_index_(std::exchange(other.write_index_, 0))
  , read_index_(std::exchange(other.read_index_, 0)) {
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::operator=(FixedCircularBuffer&& other) noexcept
    -> FixedCircularBuffer& {
  if (this != &other) {
    data_ = std::move(data_);
    write_index_ = std::exchange(other.write_index_, 0);
    read_index_ = std::exchange(other.read_index_, 0);
  }

  return *this;
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::empty() const -> bool {
  return write_index_ == read_index_;
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::full() const -> bool {
  return size() == Capacity;
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::size() const -> std::size_t {
  if (write_index_ >= read_index_) {
    return write_index_ - read_index_;
  }
  return Capacity - read_index_ + write_index_;
}

template <typename T, std::size_t Capacity>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity>::tryPush(U&& value) -> bool {
  if (full()) {
    return false;
  }

  data_[write_index_++ & INDEX_MASK] = std::forward<U>(value);

  return true;
}

template <typename T, std::size_t Capacity>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity>::forcePush(U&& value) -> void {
  // If full, increment read_index_ to logically drop the first element
  if (full()) {
    ++read_index_;
  }
  data_[write_index_++ & INDEX_MASK] = std::forward<U>(value);
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::tryPop() -> std::optional<T> {
  std::optional<T> res;
  if (empty()) {
    return res;
  }
  res.emplace(std::move(data_[read_index_++ & INDEX_MASK]));

  return res;
}
}  // namespace heph::containers
