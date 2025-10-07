//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <vector>

namespace heph::telemetry {

class Scope {
public:
  struct Value {
    std::string robot_name;
    std::string module;
  };
  explicit Scope(std::string robot_name, std::string module);
  ~Scope();

  Scope(const Scope&) = delete;
  auto operator=(const Scope&) -> Scope& = delete;
  Scope(Scope&&) = delete;
  auto operator=(Scope&&) -> Scope& = delete;
};

[[nodiscard]] auto getCurrentScopeValue() -> const Scope::Value*;

}  // namespace heph::telemetry
