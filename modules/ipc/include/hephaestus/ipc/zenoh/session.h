//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <zenoh.h>
#include <zenoh.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/utils.h"

namespace heph::ipc::zenoh {

struct Session {
  ::zenoh::Session zenoh_session;
  Config config;
};

using SessionPtr = std::shared_ptr<Session>;

[[nodiscard]] inline auto createSession(Config config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);
  return std::make_shared<Session>(::zenoh::Session::open(std::move(zconfig)), std::move(config));
}
}  // namespace heph::ipc::zenoh
