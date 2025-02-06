//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include <absl/log/check.h>
#include <fmt/core.h>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/telemetry/log.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

namespace heph::ws_bridge {

struct BridgeConfig {
  uint16_t ws_server_listening_port;
  std::string ws_server_address;
  std::vector<std::string> ws_server_client_topic_whitelist;
  std::vector<std::string> ws_server_supported_encodings;
  bool ws_server_use_compression;

  // TODO(mfehr): REMOVE if not needed
  // be exposed.
  // std::vector<std::string> ws_server_capabilities;
  // std::unordered_map<std::string, std::string> ws_server_metadata;
  // bool ws_server_use_tls;
  // size_t ws_server_send_buffer_limit_bytes;
  // std::string ws_server_certfile;
  // std::string ws_server_keyfile;
  // std::string ws_server_session_id;

  // IPC-side config
  double ipc_spin_rate_hz;

  std::vector<std::string> ipc_topic_whitelist;
  std::vector<std::string> ipc_service_whitelist;

  // TODO(mfehr): REMOVE if not needed
  // std::vector<std::string> ipc_param_whitelist;
  // uint8_t ipc_min_qos_depth;
  // uint8_t ipc_max_qos_depth;
};

std::vector<std::regex> ParseRegexStrings(const std::vector<std::string>& regex_string_vector);

BridgeConfig LoadBridgeConfigFromYaml(const std::string& yaml_file_path);

void SaveBridgeConfigToYaml(const BridgeConfig& config, const std::string& yaml_file_path);

}  // namespace heph::ws_bridge
