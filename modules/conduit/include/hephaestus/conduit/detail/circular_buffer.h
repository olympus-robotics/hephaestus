//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <cstddef>
#include <optional>

namespace heph::conduit::detail {
template <typename T, std::size_t Capacity>
class CircularBuffer {
public:
  auto push(T t) -> bool {
    if (size_ == data_.size()) {
      return false;
    }

    data_[write_index_] = std::move(t);
    write_index_ = (write_index_ + 1) % data_.size();

    ++size_;

    return true;
  }

  auto pop() -> std::optional<T> {
    if (size_ == 0) {
      return std::nullopt;
    }

    std::optional<T> res{ std::move(data_[read_index_]) };
    read_index_ = (read_index_ + 1) % data_.size();
    --size_;

    return res;
  }

  auto size() -> std::size_t {
    return size_;
  }

private:
  std::array<T, Capacity> data_{};
  std::size_t read_index_{ 0 };
  std::size_t write_index_{ 0 };
  std::size_t size_{ 0 };
};

template <typename T>
class CircularBuffer<T, 1> {
public:
  auto push(T t) -> bool {
    if (data_.has_value()) {
      return false;
    }

    data_.emplace(std::move(t));

    return true;
  }

  auto pop() -> std::optional<T> {
    return std::exchange(data_, std::optional<T>{});
  }

  auto size() -> std::size_t {
    return data_.has_value() ? 1 : 0;
  }

private:
  std::optional<T> data_{};
};
}  // namespace heph::conduit::detail
