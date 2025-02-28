//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include <absl/log/check.h>
#include <fmt/core.h>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/telemetry/log.h>
#include <magic_enum.hpp>

#include "hephaestus/utils/protobuf_serdes.h"
#include "hephaestus/utils/ws_protocol.h"

namespace heph::ws {

std::vector<std::regex> parseRegexStrings(const std::vector<std::string>& regex_string_vector);

struct WsBridgeConfig {
  ///////////////
  // WS Server //
  ///////////////
  WsServerInfo ws_server_config = { .clientTopicWhitelistPatterns = parseRegexStrings({ ".*" }),
                                    .supportedEncodings = { "protobuf", "json" },
                                    .useCompression = true,
                                    .sendBufferLimitBytes = foxglove::DEFAULT_SEND_BUFFER_LIMIT_BYTES,
                                    .useTls = false,
                                    .certfile = "",
                                    .keyfile = "",
                                    .sessionId =
                                        "session_" +
                                        std::to_string(
                                            std::chrono::system_clock::now().time_since_epoch().count()),
                                    .capabilities = {
                                        foxglove::CAPABILITY_CLIENT_PUBLISH,
                                        // foxglove::CAPABILITY_PARAMETERS,
                                        // foxglove::CAPABILITY_PARAMETERS_SUBSCRIBE,
                                        foxglove::CAPABILITY_SERVICES, foxglove::CAPABILITY_CONNECTION_GRAPH,
                                        // foxglove::CAPABILITY_ASSETS
                                    } };
  // NOTE: Unfortunately 'address' and 'port' are not part of
  // WsServerInfo and need to be passed to the server when calling
  // "start".
  uint16_t ws_server_port = 8765;
  std::string ws_server_address = "0.0.0.0";

  /////////
  // IPC //
  /////////
  heph::ipc::zenoh::Config zenoh_config;  // Default constructor is called here

  int ipc_service_call_timeout_ms = 5000;
  bool ipc_service_service_request_async = true;

  std::vector<std::string> ipc_topic_whitelist = { ".*" };
  std::vector<std::string> ipc_topic_blacklist = {};

  std::vector<std::string> ipc_service_whitelist = { ".*" };
  std::vector<std::string> ipc_service_blacklist = {};
};
;
;

bool shouldBridgeIpcTopic(const std::string& topic, const WsBridgeConfig& config);
bool shouldBridgeIpcService(const std::string& service, const WsBridgeConfig& config);
bool shouldBridgeWsTopic(const std::string& topic, const WsBridgeConfig& config);

bool isMatch(const std::string& topic, const std::vector<std::regex>& regex_list);
bool isMatch(const std::string& topic, const std::vector<std::string>& whitelist);

WsBridgeConfig loadBridgeConfigFromYaml(const std::string& yaml_file_path);

void saveBridgeConfigToYaml(const WsBridgeConfig& config, const std::string& path);

std::string convertBridgeConfigToString(const WsBridgeConfig& config);

}  // namespace heph::ws
