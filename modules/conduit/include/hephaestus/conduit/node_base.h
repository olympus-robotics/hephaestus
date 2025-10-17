//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

namespace heph::conduit {
struct NodeBase {
  virtual ~NodeBase() = default;

  [[nodiscard]] virtual auto name() const -> std::string = 0;
};
}  // namespace heph::conduit
