//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "hephaestus/conduit/detail/awaiter.h"
#include "hephaestus/conduit/detail/input_base.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {
template <typename T, typename R, typename F, typename InputPolicy = InputPolicy<>>
class AccumulatedInputBase
  : public detail::InputBase<AccumulatedInputBase<T, R, F, InputPolicy>, T, InputPolicy::DEPTH> {
  using BaseT = detail::InputBase<AccumulatedInputBase<T, R, F, InputPolicy>, T, InputPolicy::DEPTH>;

public:
  using ValueT = T;
  using InputPolicyT = InputPolicy;

  template <typename OperationT, typename DataT>
  explicit AccumulatedInputBase(Node<OperationT, DataT>* node, F f, std::string name, R initial_value = R{})
    : BaseT(node, std::move(name)), f_{ std::move(f) }, initial_value_(std::move(initial_value)) {
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
  using Awaiter = detail::Awaiter<AccumulatedInputBase, std::decay_t<Receiver>>;

private:
  F f_;
  R initial_value_;
};

template <typename R>
concept WithValueType = requires { typename R::value_type; };

template <WithValueType R, typename InputPolicy = InputPolicy<>>
using AccumulatedInput =
    AccumulatedInputBase<typename R::value_type, R, std::function<R(typename R::value_type, R)>, InputPolicy>;
}  // namespace heph::conduit
