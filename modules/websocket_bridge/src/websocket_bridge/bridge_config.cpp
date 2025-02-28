//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge_config.h"

#include <rfl/yaml.hpp>

namespace rfl {
template <>
struct Reflector<std::regex> {
  using ReflType = std::string;

  static std::regex to(const ReflType& str) noexcept {
    return std::regex(str, std::regex_constants::ECMAScript | std::regex_constants::icase);
  }

  static ReflType from(const std::regex& v) {
    (void)v;
    // TODO(mfehr): there is currently no nice way to serialize an std::regex because std::regex is lacking
    // the API to retreive the original string.
    return ".*";
  }
};
}  // namespace rfl

namespace heph::ws_bridge {

std::vector<std::regex> parseRegexStrings(const std::vector<std::string>& regex_string_vector) {
  std::vector<std::regex> regex_vector;
  regex_vector.reserve(regex_string_vector.size());

  for (const auto& regex_string : regex_string_vector) {
    try {
      regex_vector.push_back(
          std::regex(regex_string, std::regex_constants::ECMAScript | std::regex_constants::icase));
    } catch (const std::exception& ex) {
      heph::log(heph::ERROR, "Ignoring invalid regular expression", "expression", regex_string, "error",
                ex.what());
    }
  }

  return regex_vector;
}

WsBridgeConfig loadBridgeConfigFromYaml(const std::string& yaml_file_path) {
  std::ifstream file(yaml_file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open YAML file for reading: " + yaml_file_path);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string yaml_content = buffer.str();

  auto maybe_config = rfl::yaml::read<WsBridgeConfig>(yaml_content);
  if (!maybe_config) {
    throw std::runtime_error("Failed to parse YAML from: " + yaml_file_path);
  }

  return maybe_config.value();
}

void saveBridgeConfigToYaml(const WsBridgeConfig& config, const std::string& path) {
  auto yaml_str = convertBridgeConfigToString(config);
  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }
  out << yaml_str;
  if (!out.good()) {
    throw std::runtime_error("Error while writing YAML to: " + path);
  }
}

bool isMatch(const std::string& topic, const std::vector<std::regex>& regex_list) {
  for (const auto& regex : regex_list) {
    if (std::regex_match(topic, regex)) {
      return true;
    }
  }
  return false;
}

bool isMatch(const std::string& topic, const std::vector<std::string>& regex_list) {
  auto regexes = parseRegexStrings(regex_list);
  return isMatch(topic, regexes);
}

bool shouldBridgeIpcTopic(const std::string& topic, const WsBridgeConfig& config) {
  return isMatch(topic, config.ipc_topic_whitelist) && !isMatch(topic, config.ipc_topic_blacklist);
}

bool shouldBridgeIpcService(const std::string& service, const WsBridgeConfig& config) {
  return isMatch(service, config.ipc_service_whitelist) && !isMatch(service, config.ipc_service_blacklist);
}

bool shouldBridgeWsTopic(const std::string& topic, const WsBridgeConfig& config) {
  return isMatch(topic, config.ws_server_config.clientTopicWhitelistPatterns);
}

std::string convertBridgeConfigToString(const WsBridgeConfig& config) {
  return rfl::yaml::write(config);
}

}  // namespace heph::ws_bridge
