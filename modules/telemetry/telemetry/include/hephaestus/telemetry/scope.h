//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <vector>

namespace heph::telemetry {

class Scope {
public:
  explicit Scope(std::string module);
  ~Scope();

  Scope(const Scope&) = delete;
  auto operator=(const Scope&) -> Scope& = delete;
  Scope(Scope&&) = delete;
  auto operator=(Scope&&) -> Scope& = delete;
};

[[nodiscard]] auto getModulesStack() -> const std::vector<std::string>&;

}  // namespace heph::telemetry
