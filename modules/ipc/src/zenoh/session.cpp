//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/ipc/zenoh/session.h"

#include <memory>
#include <utility>

#include <fmt/core.h>
#include <zenoh/api/config.hxx>
#include <zenoh/api/session.hxx>

#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {
namespace {
// Default config https://github.com/eclipse-zenoh/zenoh/blob/master/DEFAULT_CONFIG.json5
auto createZenohConfig(const Config& config) -> ::zenoh::Config {
  throwExceptionIf<InvalidParameterException>(config.qos && config.real_time,
                                              "cannot specify both QoS and Real-Time options");
  throwExceptionIf<InvalidParameterException>(config.protocol != Protocol::ANY && !config.router.empty(),
                                              "cannot specify both protocol and the router endpoint");

  auto zconfig = ::zenoh::Config::create_default();
  // A timestamp is add to every published message.
  zconfig.insert_json5(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");  // NOLINT(misc-include-cleaner)

  // Enable shared memory support.
  if (config.enable_shared_memory) {
    zconfig.insert_json5("transport/shared_memory/enabled", "true");
  }

  // Set node in client mode.
  if (config.mode == Mode::CLIENT) {
    zconfig.insert_json5(Z_CONFIG_MODE_KEY, R"("client")");  // NOLINT(misc-include-cleaner)
  }

  // Set multicast settings
  if (config.multicast_scouting_enabled) {
    zconfig.insert_json5("scouting/multicast/enabled", "true");  // NOLINT(misc-include-cleaner)
  }
  const auto multicast_scouting_interface = fmt::format(R"("{}")", config.multicast_scouting_interface);
  zconfig.insert_json5(Z_CONFIG_MULTICAST_INTERFACE_KEY,
                       multicast_scouting_interface);  // NOLINT(misc-include-cleaner)

  // Set the transport to UDP, but I am not sure it is the right way.
  // zconfig.insert_json5(Z_CONFIG_LISTEN_KEY, R"(["udp/localhost:7447"])");
  if (config.protocol == Protocol::UDP) {
    zconfig.insert_json5(Z_CONFIG_CONNECT_KEY, R"(["udp/0.0.0.0:0"])");  // NOLINT(misc-include-cleaner)
  } else if (config.protocol == Protocol::TCP) {
    zconfig.insert_json5(Z_CONFIG_CONNECT_KEY, R"(["tcp/0.0.0.0:0"])");  // NOLINT(misc-include-cleaner)
  }

  // Add router endpoint.
  if (!config.router.empty()) {
    const auto router_endpoint = fmt::format(R"(["tcp/{}"])", config.router);
    zconfig.insert_json5(Z_CONFIG_CONNECT_KEY, router_endpoint);  // NOLINT(misc-include-cleaner)
  }
  {
    zconfig.insert_json5("transport/unicast/qos/enabled", config.qos ? "true" : "false");
  }
  if (config.real_time) {
    zconfig.insert_json5("transport/unicast/qos/enabled", "false");
    zconfig.insert_json5("transport/unicast/lowlatency", "true");
  }

  return zconfig;
}
}  // namespace

auto createSession(Config config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);
  return std::make_shared<Session>(::zenoh::Session::open(std::move(zconfig)), std::move(config));
}

}  // namespace heph::ipc::zenoh
