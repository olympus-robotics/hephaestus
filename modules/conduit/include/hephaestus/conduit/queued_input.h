//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "hephaestus/conduit/detail/awaiter.h"
#include "hephaestus/conduit/detail/input_base.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {

template <typename T, typename InputPolicy = InputPolicy<>>
class QueuedInput : public detail::InputBase<QueuedInput<T, InputPolicy>, T, InputPolicy::DEPTH> {
  using BaseT = detail::InputBase<QueuedInput<T, InputPolicy>, T, InputPolicy::DEPTH>;

public:
  using ValueT = T;
  using InputPolicyT = InputPolicy;
  template <typename OperationT, typename DataT>
  explicit QueuedInput(Node<OperationT, DataT>* node, std::string name) : BaseT(node, std::move(name)) {
  }

  auto peekValue() -> std::optional<T> {
    return this->buffer_.peek();
  }

  auto getValue() -> std::optional<T> {
    return this->buffer_.pop();
  }

  template <typename Receiver, bool Peek>
  using Awaiter = detail::Awaiter<QueuedInput, std::decay_t<Receiver>, Peek>;
};
}  // namespace heph::conduit
