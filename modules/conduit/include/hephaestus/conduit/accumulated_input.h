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
#include <vector>

#include "hephaestus/conduit/detail/awaiter.h"
#include "hephaestus/conduit/detail/input_base.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {
template <typename T, typename R, typename F, typename InputPolicy = InputPolicy<>>
class AccumulatedTransformInputBase
  : public detail::InputBase<AccumulatedTransformInputBase<T, R, F, InputPolicy>, T, InputPolicy::DEPTH> {
  using BaseT = detail::InputBase<AccumulatedTransformInputBase<T, R, F, InputPolicy>, T, InputPolicy::DEPTH>;

public:
  using ValueT = T;
  using InputPolicyT = InputPolicy;

  template <typename OperationT, typename DataT>
  explicit AccumulatedTransformInputBase(Node<OperationT, DataT>* node, F f, std::string name,
                                         R initial_value = R{})
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
      res = f_(std::move(element.value()), res);
    }
    return res;
  }

  template <typename Receiver>
  using Awaiter = detail::Awaiter<AccumulatedTransformInputBase, std::decay_t<Receiver>>;

private:
  F f_;
  R initial_value_;
};

template <typename T, typename R, typename InputPolicy = InputPolicy<>>
using AccumulatedTransformInput = AccumulatedTransformInputBase<T, R, std::function<R(T, R&)>, InputPolicy>;

namespace internal {
template <typename T>
auto accumulator(T value, std::vector<T>& state) -> std::vector<T> {
  state.push_back(value);
  return state;
}
}  // namespace internal

template <typename T, typename InputPolicy = InputPolicy<>>
class AccumulatedInput
  : public AccumulatedTransformInputBase<T, std::vector<T>, decltype(&internal::accumulator<T>),
                                         InputPolicy> {
  using BaseT =
      AccumulatedTransformInputBase<T, std::vector<T>, decltype(&internal::accumulator<T>), InputPolicy>;

public:
  template <typename OperationT, typename DataT>
  explicit AccumulatedInput(Node<OperationT, DataT>* node, std::string name,
                            std::vector<T> initial_value = std::vector<T>{})
    : BaseT(node, &internal::accumulator<T>, std::move(name), std::move(initial_value)) {
  }
};
}  // namespace heph::conduit
