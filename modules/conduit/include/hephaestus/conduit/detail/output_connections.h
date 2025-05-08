//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <stdexec/execution.hpp>

#include "hephaestus/conduit/node_engine.h"

namespace heph::conduit::detail {
struct OutputConnections {
  auto propagate(NodeEngine& /*engine*/) {
    return stdexec::let_value([this]<typename... Ts>(Ts&&... /*ts*/) {
      (void)this;
      // TODO: implement forwarding logic. As we don't have inputs yet,
      // we can't have outputs....

      // If the continuation didn't get any parameters, the operator
      // return void, and we can move on ...
      // if constexpr (sizeof...(Ts) == 0) {
      //  // Otherwise, we attempt to set the result to connected inputs.
      //} else {
      //}
    });
  }
};
}  // namespace heph::conduit::detail
