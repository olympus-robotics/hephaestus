//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/utils.h"

#include <fmt/core.h>
#include <zenoh.h>
#include <zenoh.hxx>
#include <zenohcxx/api.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

// Default config https://github.com/eclipse-zenoh/zenoh/blob/master/DEFAULT_CONFIG.json5
auto createZenohConfig(const Config& config) -> zenohc::Config {
  throwExceptionIf<InvalidParameterException>(config.qos && config.real_time,
                                              "cannot specify both QoS and Real-Time options");
  throwExceptionIf<InvalidParameterException>(config.protocol != Protocol::ANY && !config.router.empty(),
                                              "cannot specify both protocol and the router endpoint");

  zenohc::Config zconfig;
  // A timestamp is add to every published message.
  zconfig.insert_json(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");

  // Enable shared memory support.
  if (config.enable_shared_memory) {
    zconfig.insert_json("transport/shared_memory/enabled", "true");
  }

  // Set node in client mode.
  if (config.mode == Mode::CLIENT) {
    const bool res = zconfig.insert_json(Z_CONFIG_MODE_KEY, R"("client")");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to set node mode as client");
  }

  // Set the transport to UDP, but I am not sure it is the right way.
  // zconfig.insert_json(Z_CONFIG_LISTEN_KEY, R"(["udp/localhost:7447"])");
  if (config.protocol == Protocol::UDP) {
    const auto res = zconfig.insert_json(Z_CONFIG_CONNECT_KEY, R"(["udp/0.0.0.0:0"])");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to add UDP connection");
  } else if (config.protocol == Protocol::TCP) {
    const auto res = zconfig.insert_json(Z_CONFIG_CONNECT_KEY, R"(["tcp/0.0.0.0:0"])");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to add TCP connection");
  }

  // Add router endpoint.
  if (!config.router.empty()) {
    const auto router_endpoint = fmt::format(R"(["tcp/{}"])", config.router);
    const auto res = zconfig.insert_json(Z_CONFIG_CONNECT_KEY, router_endpoint.c_str());
    throwExceptionIf<FailedZenohOperation>(!res, "failed to add router endpoint");
  }
  {
    const auto res = zconfig.insert_json("transport/unicast/qos/enabled", config.qos ? "true" : "false");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to set QoS");
  }
  if (config.real_time) {
    auto res = zconfig.insert_json("transport/unicast/qos/enabled", "false");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to set QoS");
    res = zconfig.insert_json("transport/unicast/lowlatency", "true");
    throwExceptionIf<FailedZenohOperation>(!res, "failed to set lowlatency");
  }

  return zconfig;
}
}  // namespace heph::ipc::zenoh
