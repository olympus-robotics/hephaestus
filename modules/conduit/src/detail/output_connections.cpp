
//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#if 0

#include "hephaestus/conduit/detail/output_connections.h"

#include <string>
#include <utility>

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

auto OutputConnections::rawName() const -> std::string {
  return name_;
}

void OutputConnections::registerInputToEngine(std::string input_name, std::string input_type,
                                              detail::NodeBase* node) {
  if (node->enginePtr() == nullptr) {
    return;
  }

  node_->engine().addConnectionSpec({
      .input = {
          .name = std::move(input_name),
          .node_name = node->nodeName(),  // Include the node name for better identification
          .type = std::move(input_type),

      },
      .output = {
          .name = name_,
          .node_name  = node_->nodeName(),
          .type = "",

      },
  });
}

}  // namespace heph::conduit::detail
#endif
