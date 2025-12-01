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
///
/// \tparam T the value type of the elements
/// \tparam Capacity Maximum capacity. Should be a power of two for efficiency reasons.
///
/// \note This class is not thread safe. If you need to push/pop from differeent threads, protect it with a
/// lock!
template <typename T, std::size_t Capacity>
class FixedCircularBuffer {
public:
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity should be a power of two.");

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
  return size_ == 0;
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
inline auto FixedCircularBuffer<T, Capacity>::push(U&& value) -> bool {
  if (full()) {
    return false;
  }

  data_[write_index_] = std::forward<U>(value);
  write_index_ = (write_index_ + 1) % Capacity;
  ++size_;

  return true;
}

template <typename T, std::size_t Capacity>
template <std::convertible_to<T> U>
inline auto FixedCircularBuffer<T, Capacity>::pushForce(U&& u) -> void {
  while (!push(std::forward<U>(u))) {
    [[maybe_unused]] auto popped_element = pop();
    assert(popped_element.has_value());
  }
}

template <typename T, std::size_t Capacity>
inline auto FixedCircularBuffer<T, Capacity>::pop() -> std::optional<T> {
  std::optional<T> res;
  if (empty()) {
    return res;
  }
  res.emplace(std::move(data_[read_index_]));
  read_index_ = (read_index_ + 1) % Capacity;
  --size_;

  return res;
}
}  // namespace heph::containers
