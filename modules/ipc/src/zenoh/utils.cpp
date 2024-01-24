//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {
auto createZenohConfig(Config& config) -> zenohc::Config {
  zenohc::Config zconfig;
  // A timestamp is add to every published message.
  zconfig.insert_json(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");

  // Enable shared memory support.
  if (config.enable_shared_memory) {
    zconfig.insert_json("transport/shared_memory/enabled", "true");
  }

  // Set node in client mode.
  if (config.mode == Config::CLIENT) {
    static constexpr std::string_view DEFAULT_ROUTER = "localhost:7447";
    if (config.router.empty()) {
      config.router = DEFAULT_ROUTER;
    }

    zconfig.insert_json(Z_CONFIG_MODE_KEY, R"("client")");
  }

  // Add router endpoint.
  if (!config.router.empty()) {
    const auto router_endpoint = std::format(R"(["tcp/{}"])", config.router);
    zconfig.insert_json(Z_CONFIG_CONNECT_KEY, router_endpoint.c_str());
  }

  return zconfig;
}
}  // namespace eolo::ipc::zenoh
