//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/utils.h"

#include "eolo/base/exception.h"

namespace eolo::ipc::zenoh {

// Default config https://github.com/eclipse-zenoh/zenoh/blob/master/DEFAULT_CONFIG.json5
auto createZenohConfig(const Config& config) -> zenohc::Config {
  zenohc::Config zconfig;
  // A timestamp is add to every published message.
  zconfig.insert_json(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");

  // Enable shared memory support.
  if (config.enable_shared_memory) {
    zconfig.insert_json("transport/shared_memory/enabled", "true");
  }

  // Set node in client mode.
  if (config.mode == Mode::CLIENT) {
    bool res = zconfig.insert_json(Z_CONFIG_MODE_KEY, R"("client")");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to set node mode as client");
  }

  // Set the transport to UDP, but I am not sure it is the right way.
  // zconfig.insert_json(Z_CONFIG_LISTEN_KEY, R"(["udp/localhost:7447"])");

  // Add router endpoint.
  if (!config.router.empty()) {
    const auto router_endpoint = std::format(R"(["tcp/{}"])", config.router);
    auto res = zconfig.insert_json(Z_CONFIG_CONNECT_KEY, router_endpoint.c_str());
    throwExceptionIf<FailedZenohOperation>(!res, "failed to add router endpoint");
  }

  return zconfig;
}
}  // namespace eolo::ipc::zenoh
