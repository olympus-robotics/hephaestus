
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
  return fmt::format("{}/{}", node_->nodeName(), name_);
}

void OutputConnections::registerInputToEngine(const std::string& name, std::string type,
                                              detail::NodeBase* node) {
  if (node->enginePtr() == nullptr) {
    return;
  }

  auto input_name = name;
  // If the input contains the node name, we need to extract it.
  const auto pos = name.find(node->nodeName());
  if (pos != std::string::npos) {
    input_name =
        name.substr(node->nodeName().size() + 1);  // Remove the node name plus the separator character
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
