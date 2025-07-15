
//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/detail/output_connections.h"

#include <string>

#include <fmt/format.h>

#include "hephaestus/conduit/detail/node_base.h"

namespace heph::conduit::detail {

auto OutputConnections::name() const -> std::string {
  return fmt::format("{}/{}", node_->nodeName(), name_);
}

}  // namespace heph::conduit::detail
