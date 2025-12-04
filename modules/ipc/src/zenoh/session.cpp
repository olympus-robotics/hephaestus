//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/ipc/zenoh/session.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <unistd.h>
#include <zenoh.h>
#include <zenoh/api/config.hxx>
#include <zenoh/api/session.hxx>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/utils/string/string_utils.h"
#include "hephaestus/utils/utils.h"

namespace heph::ipc::zenoh {
namespace {

void removeInvalidChar(std::string& str) {
  const auto [first, last] = std::ranges::remove_if(str, [](char c) { return !isValidIdChar(c); });
  str.erase(first, last);
}

[[nodiscard]] auto createSessionId(std::string_view id) -> std::string {
  static constexpr std::size_t MAX_SESSION_ID_SIZE = 16;

  HEPH_PANIC_IF(!isValidId(id), "invalid session id: {}, only alphanumeric and _ characters allowed", id);

  std::string session_id{ id };

  if (session_id.size() > MAX_SESSION_ID_SIZE) {
    heph::log(heph::WARN, "session id is too long, truncating", "session_id", session_id, "max_size",
              MAX_SESSION_ID_SIZE);
    session_id = session_id.substr(0, MAX_SESSION_ID_SIZE);
  }

  // Zenoh reverse this.
  std::ranges::reverse(session_id);
  return utils::string::toAsciiHex(session_id);
}

[[nodiscard]] auto createSessionIdFromBinaryName() -> std::string {
  auto binary_name = utils::getBinaryPath();
  HEPH_PANIC_IF(!binary_name.has_value(), "cannot get binary name");
  std::string filename = binary_name->filename();  // NOLINT(bugprone-unchecked-optional-access);
  removeInvalidChar(filename);
  filename = fmt::format("{}_{}", getpid(), filename);
  return createSessionId(filename);
}

// Default config https://github.com/eclipse-zenoh/zenoh/blob/master/DEFAULT_CONFIG.json5
auto createZenohConfig(const Config& config) -> ::zenoh::Config {
  if (config.zenoh_config_path.has_value()) {
    return std::move(ZenohConfig{ *config.zenoh_config_path }.zconfig);
  }

  HEPH_PANIC_IF(config.qos && config.real_time, "cannot specify both QoS and Real-Time options");
  HEPH_PANIC_IF(config.protocol != Protocol::ANY && !config.router.empty(),
                "cannot specify both protocol and the router endpoint");

  auto zconfig = ZenohConfig{};
  if (config.use_binary_name_as_session_id) {
    setSessionIdFromBinary(zconfig);
  } else if (config.id.has_value()) {
    setSessionId(zconfig, config.id.value());
  }

  // Enable shared memory support.
  setSharedMemory(zconfig, config.enable_shared_memory);

  setMode(zconfig, config.mode);

  // Set multicast settings
  setMulticastScouting(zconfig, config.multicast_scouting_enabled);
  setMulticastScoutingInterface(zconfig, config.multicast_scouting_interface);

  // Set the transport to UDP, but I am not sure it is the right way.
  // zconfig.insert_json5(Z_CONFIG_LISTEN_KEY, R"(["udp/localhost:7447"])");
  if (config.protocol == Protocol::UDP) {
    connectToEndpoints(zconfig, { "udp/0.0.0.0:0" });
  } else if (config.protocol == Protocol::TCP) {
    connectToEndpoints(zconfig, { "tcp/0.0.0.0:0" });
  }

  // Add router endpoint.
  if (!config.router.empty()) {
    connectToEndpoints(zconfig, { fmt::format("tcp/{}", config.router) });
  }
  setQos(zconfig, config.qos);
  if (config.real_time) {
    setRealTime(zconfig, true);
  }

  return std::move(zconfig.zconfig);
}

auto toJsonArray(const std::vector<std::string>& values) -> std::string {
  return fmt::format("[{}]", fmt::join(values | std::views::transform([](const std::string& value) {
                                         return fmt::format(R"("{}")", value);
                                       }),
                                       ","));
}
}  // namespace

ZenohConfig::ZenohConfig() {
  zconfig.insert_json5(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");  // NOLINT(misc-include-cleaner)
}

ZenohConfig::ZenohConfig(const std::filesystem::path& path) : zconfig(::zenoh::Config::from_file(path)) {
}

void setSessionId(ZenohConfig& config, std::string_view id) {
  config.zconfig.insert_json5("id", fmt::format(R"("{}")", createSessionId(id)));
}

void setSessionIdFromBinary(ZenohConfig& config) {
  config.zconfig.insert_json5("id", fmt::format(R"("{}")", createSessionIdFromBinaryName()));
}

void setSharedMemory(ZenohConfig& config, bool value) {
  config.zconfig.insert_json5("transport/shared_memory/enabled", fmt::format("{}", value));
}

void setMode(ZenohConfig& config, Mode mode) {
  switch (mode) {
    case Mode::PEER:
      config.zconfig.insert_json5(Z_CONFIG_MODE_KEY, R"("peer")");  // NOLINT(misc-include-cleaner)
      break;
    case Mode::CLIENT:
      config.zconfig.insert_json5(Z_CONFIG_MODE_KEY, R"("client")");  // NOLINT(misc-include-cleaner)
      break;
    case Mode::ROUTER:
      config.zconfig.insert_json5(Z_CONFIG_MODE_KEY, R"("router")");  // NOLINT(misc-include-cleaner)
      break;
    default:
      std::abort();
  }
}

void connectToEndpoints(ZenohConfig& config, const std::vector<std::string>& endpoints) {
  config.zconfig.insert_json5(Z_CONFIG_CONNECT_KEY, toJsonArray(endpoints));  // NOLINT(misc-include-cleaner)
}

void listenToEndpoints(ZenohConfig& config, const std::vector<std::string>& endpoints) {
  config.zconfig.insert_json5(Z_CONFIG_LISTEN_KEY, toJsonArray(endpoints));  // NOLINT(misc-include-cleaner)
}

void setQos(ZenohConfig& config, bool value) {
  config.zconfig.insert_json5("transport/unicast/qos/enabled", fmt::format("{}", value));
}

void setRealTime(ZenohConfig& config, bool value) {
  if (value) {
    config.zconfig.insert_json5("transport/unicast/qos/enabled", "false");
    config.zconfig.insert_json5("transport/unicast/lowlatency", "true");
  } else {
    config.zconfig.insert_json5("transport/unicast/qos/enabled", "true");
    config.zconfig.insert_json5("transport/unicast/lowlatency", "false");
  }
}

void setMulticastScouting(ZenohConfig& config, bool value) {
  config.zconfig.insert_json5("scouting/multicast/enabled", fmt::format("{}", value));
}

void setMulticastScoutingInterface(ZenohConfig& config, std::string_view interface) {
  config.zconfig.insert_json5("scouting/multicast/interface", fmt::format(R"("{}")", interface));
}

auto createLocalConfig() -> Config {
  Config config;
  config.multicast_scouting_enabled = false;
  return config;
}

auto createSession(const Config& config) -> SessionPtr {
  auto zconfig = createZenohConfig(config);
  return std::make_shared<Session>(::zenoh::Session::open(std::move(zconfig)));
}

auto createSession(ZenohConfig config) -> SessionPtr {
  return std::make_shared<Session>(::zenoh::Session::open(std::move(config.zconfig)));
}
}  // namespace heph::ipc::zenoh
