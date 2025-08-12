
//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/detail/output_connections.h"

#include <span>
#include <string>
#include <utility>
#include <vector>

#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>
#include <fmt/format.h>

#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/node_engine.h"

namespace heph::conduit::detail {

OutputConnections::OutputConnections(detail::NodeBase* node, std::string name)
  : node_(node), name_(std::move(name)) {
}

auto OutputConnections::name() const -> std::string {
  if (node_->enginePtr() == nullptr) {
    return name("");
  }
  return name(node_->engine().prefix());
}

auto OutputConnections::name(const std::string& prefix) const -> std::string {
  return fmt::format("{}/{}", node_->nodeName(prefix), name_);
}

void OutputConnections::registerInputToEngine(const std::string& name, std::string type,
                                              detail::NodeBase* node) {
  if (node->enginePtr() == nullptr) {
    return;
  }

  std::string input_name;
  std::vector<std::string> name_parts = absl::StrSplit(name, '/');
  if (name_parts.size() == 1) {
    input_name = name_parts.front();
  } else if (name_parts.size() == 2) {
    input_name = name_parts.back();
  } else {
    input_name = absl::StrJoin(std::span{ name_parts }.subspan(1), "/");
  }
  node_->engine().addConnectionSpec({
      .input = {
          .name = std::move(input_name),
          .node_name = node->nodeName(),  // Include the node name for better identification
          .type = std::move(type),

      },
      .output = {
          .name = name_,
          .node_name  = node_->nodeName(),
          .type = "",

      },
  });
}

}  // namespace heph::conduit::detail
