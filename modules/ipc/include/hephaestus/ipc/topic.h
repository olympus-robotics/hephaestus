//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

namespace heph::ipc {

struct TopicConfig {
  explicit TopicConfig(std::string name);
  std::string name;
};

}  // namespace heph::ipc
