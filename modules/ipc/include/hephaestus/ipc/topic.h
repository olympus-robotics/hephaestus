//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

namespace heph::ipc {

struct TopicConfig {
  /// Construct a TopicConfig with the given name.
  /// The constructor will throw an exception if the name is not a valid zenoh topic name.
  explicit TopicConfig(std::string name);
  std::string name;
};

}  // namespace heph::ipc
