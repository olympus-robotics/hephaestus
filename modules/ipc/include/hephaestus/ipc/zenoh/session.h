//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/utils.h"

namespace heph::ipc::zenoh {

struct Session {
  zenohc::Session zenoh_session;
  Config config;
};

using SessionPtr = std::shared_ptr<Session>;

[[nodiscard]] inline auto createSession(Config&& config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);
  auto session = std::make_shared<Session>(
      ::heph::ipc::zenoh::expect<zenohc::Session>(open(std::move(zconfig))), std::move(config));

  return session;
}
}  // namespace heph::ipc::zenoh
