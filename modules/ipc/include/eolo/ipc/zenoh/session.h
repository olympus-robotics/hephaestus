//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

using SessionPtr = std::shared_ptr<zenohc::Session>;

[[nodiscard]] inline auto createSession(const Config& config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);

  return expectAsSharedPtr(open(std::move(zconfig)));
}
}  // namespace eolo::ipc::zenoh
