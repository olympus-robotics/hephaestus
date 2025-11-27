//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

namespace heph::containers {

/// Implementation of a fixed capacity circular buffer
template <typename T, std::size_t Capacity>
class FixedCircularBuffer {
public:
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
  /// \return std::nullopt is buffer is empty, an optional containing the value otherwise
  [[nodiscard]] auto pop() -> std::optional<T>;

private:
  std::array<T, Capacity> data_{};
  std::size_t write_index_{ 0 };
  std::size_t read_index_{ 0 };
  std::size_t size_{ 0 };
};

template <typename T, std::size_t Capacity>
inline FixedCircularBuffer<T, Capacity>::FixedCircularBuffer(FixedCircularBuffer&& other) noexcept
  : data_(std::move(data_))
  , write_index_(std::exchange(other.write_index_, 0))
  , read_index_(std::exchange(other.read_index_, 0))
  , size_(std::exchange(other.size_, false)) {
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::operator=(FixedCircularBuffer&& other) noexcept
    -> FixedCircularBuffer& {
  if (this != &other) {
    data_ = std::move(data_);
    write_index_ = std::exchange(other.write_index_, 0);
    read_index_ = std::exchange(other.read_index_, 0);
    size_ = std::exchange(other.size_, 0);
  }

  return *this;
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::empty() const -> bool {
  if (full()) {
    return false;
  }
  return write_index_ == read_index_;
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::full() const -> bool {
  return size_ == Capacity;
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::size() const -> std::size_t {
  return size_;
}

template <typename T, std::size_t Capacity>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity>::push(U&& u) -> bool {
  if (full()) {
    return false;
  }

  data_[write_index_] = std::forward<U>(u);
  write_index_ = (write_index_ + 1) % Capacity;
  ++size_;

  return true;
}

template <typename T, std::size_t Capacity>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity>::pushForce(U&& u) -> void {
  while (!push(std::forward<U>(u))) {
    (void)pop();
  }
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::pop() -> std::optional<T> {
  if (size_ == 0) {
    return std::nullopt;
  }

  std::optional<T> res{ std::move(data_[read_index_]) };
  read_index_ = (read_index_ + 1) % Capacity;
  --size_;

  return res;
}
}  // namespace heph::containers
