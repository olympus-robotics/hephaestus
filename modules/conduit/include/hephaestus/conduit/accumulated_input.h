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
template <typename T, typename R, typename F, typename InputPolicy = InputPolicy<2>>
class AccumulatedInput : detail::InputBase<AccumulatedInput<T, F, InputPolicy>> {
  using BaseT = detail::InputBase<AccumulatedInput<T, F, InputPolicy>>;

public:
  using ValueT = T;
  using InputPolicyT = InputPolicy;
  static_assert(InputPolicyT::DEPTH >= 2, "Please specify a Depth policy larger than 2");

  template <typename OperationT>
  explicit AccumulatedInput(Node<OperationT>* node, F f, R initial_value = R{})
    // TODO: pass name
    : BaseT(""), node_(node), f_{ std::move(f) }, initial_value_(std::move(initial_value)) {
  }

  template <typename U>
  auto setValue(U&& u) -> InputState {
    if (size_ == data_.size()) {
      return InputState::OVERFLOW;
    }
    auto index = (current_ + size_) % data_.size();
    data_[index] = std::forward<U>(u);
    ++size_;
    if (size_ > 1) {
      this->triggerAwaiter();
    }
    return InputState::OK;
  }

  auto getValue() -> std::optional<R> {
    if (size_ < 2) {
      return std::nullopt;
    }
    R res{ initial_value_ };
    while (size_ != 0) {
      auto index = current_;
      current_ = (current_ + 1) % data_.size();
      --size_;
      res = f_(data_.at(index), res);
    }
    return std::optional<R>{ std::move(res) };
  }

  template <typename Receiver>
  using Awaiter = detail::Awaiter<AccumulatedInput, std::decay_t<Receiver>>;

private:
  std::array<T, InputPolicyT::DEPTH> data_{};
  std::size_t size_{ 0 };
  std::size_t current_{ 0 };
  F f_;
  R initial_value_;

  void* node_{ nullptr };
};
}  // namespace heph::conduit
