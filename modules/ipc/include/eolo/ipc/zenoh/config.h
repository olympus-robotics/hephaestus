//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <expected>
#include <zenohc.hxx>

namespace eolo::ipc::zenoh {
struct Config {
  enum Mode { PEER, CLIENT };
  std::string topic;
  bool enable_shared_memory = false;  //! NOTE: With shared-memory enabled, the publisher still uses the
                                      //! network transport layer to notify subscribers of the shared-memory
                                      //! segment to read. Therefore, for very small messages, shared -
                                      //! memory transport could be less efficient than using the default
                                      //! network transport to directly carry the payload.
  Mode mode = PEER;
  // NOLINTNEXTLINE(readability-redundant-string-init) otherwise we need to specify in constructor
  std::string router = "";  //! If specified connect to the given router endpoint.
  std::size_t cache_size = 0;
};

[[nodiscard]] inline auto createZenohConfig(Config& config) -> zenohc::Config {
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
