//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <regex>
#include <string>
#include <vector>

#include <absl/log/check.h>
#include <fmt/core.h>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/telemetry/log.h>
#include <magic_enum.hpp>

#include "hephaestus/utils/ws_protocol.h"

namespace heph::ws {

[[nodiscard]] auto parseRegexStrings(const std::vector<std::string>& regex_string_vector)
    -> std::vector<std::regex>;

struct WsBridgeConfig {
  ///////////////
  // WS Server //
  ///////////////
  WsServerInfo ws_server_config = {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    .capabilities = {
      foxglove::CAPABILITY_CLIENT_PUBLISH,
      // foxglove::CAPABILITY_PARAMETERS,
      // foxglove::CAPABILITY_PARAMETERS_SUBSCRIBE,
      foxglove::CAPABILITY_SERVICES,
      foxglove::CAPABILITY_CONNECTION_GRAPH,
      // foxglove::CAPABILITY_ASSETS
    },
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    .supportedEncodings = {},
    .metadata = {},
    .sendBufferLimitBytes = foxglove::DEFAULT_SEND_BUFFER_LIMIT_BYTES,
    .useTls = false,
    .certfile = "",
    .keyfile = "",
    .sessionId =
        "session_" +
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count()),
    .numWorkerThreads = 1,
    .useCompression = true,
    .clientTopicWhitelistPatterns = parseRegexStrings({ ".*" }),
  };
  // NOTE: Unfortunately 'address' and 'port' are not part of
  // WsServerInfo and need to be passed to the server when calling
  // "start".
  static constexpr uint16_t DEFAULT_WS_SERVER_PORT = 8765;
  uint16_t ws_server_port = DEFAULT_WS_SERVER_PORT;
  std::string ws_server_address = "0.0.0.0";

  /////////
  // IPC //
  /////////
  heph::ipc::zenoh::Config zenoh_config;  // Default constructor is called here

  static constexpr int DEFAULT_IPC_SERVICE_CALL_TIMEOUT_MS = 5000;
  int ipc_service_call_timeout_ms = DEFAULT_IPC_SERVICE_CALL_TIMEOUT_MS;
  bool ipc_service_service_request_async = true;

  std::vector<std::string> ipc_topic_whitelist = { ".*" };
  std::vector<std::string> ipc_topic_blacklist;

  std::vector<std::string> ipc_service_whitelist = { ".*" };
  std::vector<std::string> ipc_service_blacklist;
};

[[nodiscard]] auto shouldBridgeIpcTopic(const std::string& topic, const WsBridgeConfig& config) -> bool;
[[nodiscard]] auto shouldBridgeIpcService(const std::string& service, const WsBridgeConfig& config) -> bool;
[[nodiscard]] auto shouldBridgeWsTopic(const std::string& topic, const WsBridgeConfig& config) -> bool;

[[nodiscard]] auto isMatch(const std::string& topic, const std::vector<std::regex>& regex_list) -> bool;
[[nodiscard]] auto isMatch(const std::string& topic, const std::vector<std::string>& regex_string_list) -> bool;

[[nodiscard]] auto loadBridgeConfigFromYaml(const std::string& yaml_file_path) -> WsBridgeConfig;

void saveBridgeConfigToYaml(const WsBridgeConfig& config, const std::string& path);

auto convertBridgeConfigToString(const WsBridgeConfig& config) -> std::string;

}  // namespace heph::ws
