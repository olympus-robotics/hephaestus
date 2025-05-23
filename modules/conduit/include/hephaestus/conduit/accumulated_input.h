//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "hephaestus/conduit/detail/awaiter.h"
#include "hephaestus/conduit/detail/input_base.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {
template <typename T, typename R, typename F, typename InputPolicy = InputPolicy<>>
class AccumulatedInput
  : public detail::InputBase<AccumulatedInput<T, F, InputPolicy>, T, InputPolicy::DEPTH> {
  using BaseT = detail::InputBase<AccumulatedInput<T, F, InputPolicy>, T, InputPolicy::DEPTH>;

public:
  using ValueT = T;
  using InputPolicyT = InputPolicy;

  template <typename OperationT>
  explicit AccumulatedInput(Node<OperationT>* node, F f, std::string_view name, R initial_value = R{})
    : BaseT(node, fmt::format("{}/{}", node->nodeName(), name))
    , f_{ std::move(f) }
    , initial_value_(std::move(initial_value)) {
  }

  auto getValue() -> std::optional<R> {
    if (this->buffer_.size() == 0) {
      return std::nullopt;
    }
    R res{ initial_value_ };
    while (true) {
      auto element = this->buffer_.pop();
      if (!element.has_value()) {
        break;
      }
      res = f_(*element, res);
    }
    return res;
  }

  template <typename Receiver>
  using Awaiter = detail::Awaiter<AccumulatedInput, std::decay_t<Receiver>>;

private:
  F f_;
  R initial_value_;
};
}  // namespace heph::conduit
