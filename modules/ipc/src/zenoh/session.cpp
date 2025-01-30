//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/ipc/zenoh/session.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <fmt/format.h>
#include <zenoh/api/config.hxx>
#include <zenoh/api/session.hxx>

#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::ipc::zenoh {
namespace {

// Default config https://github.com/eclipse-zenoh/zenoh/blob/master/DEFAULT_CONFIG.json5
auto createZenohConfig(const Config& config) -> ::zenoh::Config {
  throwExceptionIf<InvalidParameterException>(config.qos && config.real_time,
                                              "cannot specify both QoS and Real-Time options");
  throwExceptionIf<InvalidParameterException>(config.protocol != Protocol::ANY && !config.router.empty(),
                                              "cannot specify both protocol and the router endpoint");

  auto zconfig = ::zenoh::Config::create_default();

  if (config.id.has_value()) {
    static constexpr std::size_t MAX_SESSION_ID_SIZE = 16;
    auto session_id = config.id.value();
    if (session_id.size() > MAX_SESSION_ID_SIZE) {
      heph::log(heph::WARN, "session id is too long, truncating", "session_id", session_id, "max_size",
                MAX_SESSION_ID_SIZE);
      session_id = session_id.substr(0, MAX_SESSION_ID_SIZE);
    }

    // Zenoh reverse this.
    std::ranges::reverse(session_id);
    const auto hex_str = utils::string::toHex(session_id);
    zconfig.insert_json5("id", fmt::format(R"("{}")", hex_str));  // NOLINT(misc-include-cleaner)
  }

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
  zconfig.insert_json5("scouting/multicast/interface",
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

auto createLocalConfig() -> Config {
  Config config;
  config.multicast_scouting_enabled = false;
  return config;
}

auto createSession(Config config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);
  return std::make_shared<Session>(::zenoh::Session::open(std::move(zconfig)), std::move(config));
}

}  // namespace heph::ipc::zenoh
