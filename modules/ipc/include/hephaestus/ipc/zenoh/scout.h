//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <vector>

#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

struct NodeInfo {
  // TODO: update this to a uuid.
  std::string id;
  Mode mode;
  std::vector<std::string> locators;
};

[[nodiscard]] auto getListOfNodes() -> std::vector<NodeInfo>;

[[nodiscard]] auto toString(const NodeInfo& info) -> std::string;
}  // namespace heph::ipc::zenoh
