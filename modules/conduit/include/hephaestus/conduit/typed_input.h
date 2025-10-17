//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string_view>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/basic_input.h"

namespace heph::conduit {
template <typename T>
struct TypedInput : BasicInput {
  using SenderT = BasicInput::SenderT;
  using SetValueSenderT = concurrency::AnySender<void>;

  explicit TypedInput(std::string_view name) : BasicInput(name) {
  }

  virtual auto setValue(T t) -> SetValueSenderT = 0;
};
}  // namespace heph::conduit
