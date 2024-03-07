//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

struct Session {
  zenohc::Session zenoh_session;
  Config config;
};

using SessionPtr = std::shared_ptr<Session>;

[[nodiscard]] inline auto createSession(Config&& config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);
  auto session = std::make_shared<Session>(
      ::eolo::ipc::zenoh::expect<zenohc::Session>(open(std::move(zconfig))), std::move(config));

  return session;
}
}  // namespace eolo::ipc::zenoh
