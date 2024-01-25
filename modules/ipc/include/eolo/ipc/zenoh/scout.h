//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once
#include <vector>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/liveliness.h"

namespace eolo::ipc::zenoh {

struct NodeInfo {
  // TODO: update this to a uuid.
  std::string id;
  Mode mode;
  std::vector<std::string> locators;
};

[[nodiscard]] auto getListOfNodes() -> std::vector<NodeInfo>;

[[nodiscard]] auto toString(const NodeInfo& info) -> std::string;
}  // namespace eolo::ipc::zenoh
