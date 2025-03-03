//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge_config.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <hephaestus/telemetry/log.h>
#include <rfl.hpp>
#include <rfl/internal/has_reflector.hpp>
#include <rfl/json.hpp>
#include <rfl/parsing/Parser_default.hpp>
#include <rfl/yaml/read.hpp>
#include <rfl/yaml/write.hpp>

#include "hephaestus/utils/ws_protocol.h"

namespace rfl {
template <>
struct Reflector<std::regex> {
  using ReflType = std::string;

  static std::regex to(const ReflType& str) noexcept {  // NOLINT
    return std::regex(str, std::regex_constants::ECMAScript | std::regex_constants::icase);
  }

  static ReflType from(const std::regex& v) {  // NOLINT
    (void)v;
    // TODO(mfehr): there is currently no nice way to serialize an std::regex because std::regex is lacking
    // the API to retreive the original string.
    return ".*";
  }
};
}  // namespace rfl

namespace heph::ws {

auto parseRegexStrings(const std::vector<std::string>& regex_string_vector) -> std::vector<std::regex> {
  std::vector<std::regex> regex_vector;
  regex_vector.reserve(regex_string_vector.size());

  for (const auto& regex_string : regex_string_vector) {
    try {
      regex_vector.emplace_back(regex_string, std::regex_constants::ECMAScript | std::regex_constants::icase);
    } catch (const std::exception& ex) {
      heph::log(heph::ERROR, "Ignoring invalid regular expression", "expression", regex_string, "error",
                ex.what());
    }
  }

  return regex_vector;
}

auto loadBridgeConfigFromYaml(const std::string& yaml_file_path) -> WsBridgeConfig {
  const std::ifstream file(yaml_file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open YAML file for reading: " + yaml_file_path);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string yaml_content = buffer.str();

  const auto maybe_config = rfl::yaml::read<WsBridgeConfig>(yaml_content);
  if (!maybe_config) {
    throw std::runtime_error("Failed to parse YAML from: " + yaml_file_path);
  }

  return maybe_config.value();
}

void saveBridgeConfigToYaml(const WsBridgeConfig& config, const std::string& path) {
  auto yaml_str = convertBridgeConfigToString(config);
  {
    std::ofstream out(path);
    if (!out.is_open()) {
      throw std::runtime_error("Failed to open file for writing: " + path);
    }
    out << yaml_str;
    if (!out.good()) {
      throw std::runtime_error("Error while writing YAML to: " + path);
    }
  }
}

auto isMatch(const std::string& topic, const std::vector<std::regex>& regex_list) -> bool {
  return std::ranges::any_of(regex_list,
                             [&topic](const std::regex& regex) { return std::regex_match(topic, regex); });
}

auto isMatch(const std::string& topic, const std::vector<std::string>& regex_string_list) -> bool {
  auto regex_list = parseRegexStrings(regex_string_list);
  return isMatch(topic, regex_list);
}

auto shouldBridgeIpcTopic(const std::string& topic, const WsBridgeConfig& config) -> bool {
  return isMatch(topic, config.ipc_topic_whitelist) && !isMatch(topic, config.ipc_topic_blacklist);
}

auto shouldBridgeIpcService(const std::string& service, const WsBridgeConfig& config) -> bool {
  return isMatch(service, config.ipc_service_whitelist) && !isMatch(service, config.ipc_service_blacklist);
}

auto shouldBridgeWsTopic(const std::string& topic, const WsBridgeConfig& config) -> bool {
  return isMatch(topic, config.ws_server_config.clientTopicWhitelistPatterns);
}

auto convertBridgeConfigToString(const WsBridgeConfig& config) -> std::string {
  return rfl::yaml::write(config);
}

}  // namespace heph::ws
