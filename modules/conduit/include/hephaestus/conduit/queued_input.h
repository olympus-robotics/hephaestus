//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

#include "hephaestus/conduit/detail/awaiter.h"
#include "hephaestus/conduit/detail/input_base.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {

template <typename T, typename InputPolicy = InputPolicy<>>
class QueuedInput : public detail::InputBase<QueuedInput<T, InputPolicy>> {
  using BaseT = detail::InputBase<QueuedInput<T, InputPolicy>>;

public:
  using ValueT = T;
  using InputPolicyT = InputPolicy;
  template <typename OperationT>
  explicit QueuedInput(Node<OperationT>* node, std::string name = "") : BaseT(std::move(name)), node_(node) {
  }

  template <typename U>
  auto setValue(U&& u) -> InputState {
    if (size_ == data_.size()) {
      return InputState::OVERFLOW;
    }
    auto index = (current_ + size_) % data_.size();
    data_[index] = std::forward<U>(u);
    ++size_;
    this->triggerAwaiter();
    return InputState::OK;
  }

  auto getValue() -> std::optional<T> {
    if (size_ == 0) {
      return std::nullopt;
    }
    auto index = current_;
    current_ = (current_ + 1) % data_.size();
    --size_;
    return std::optional<T>{ std::move(data_[index]) };
  }

  template <typename Receiver>
  using Awaiter = detail::Awaiter<QueuedInput, std::decay_t<Receiver>>;

private:
  std::array<T, InputPolicyT::DEPTH> data_{};
  std::size_t size_{ 0 };
  std::size_t current_{ 0 };

  void* node_{ nullptr };
};
}  // namespace heph::conduit
