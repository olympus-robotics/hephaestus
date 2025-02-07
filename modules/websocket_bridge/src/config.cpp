#include "hephaestus/websocket_bridge/config.h"

#include <yaml-cpp/yaml.h>

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
  YAML::Node yaml_data = YAML::LoadFile(yaml_file_path);

  WsBridgeConfig config;
  const auto& ws_server = yaml_data["ws_server"];
  config.ws_server_listening_port = ws_server["listening_port"].as<uint16_t>();
  config.ws_server_address = ws_server["address"].as<std::string>();
  config.ws_server_client_topic_whitelist =
      ws_server["client_topic_whitelist"].as<std::vector<std::string>>();
  config.ws_server_supported_encodings = ws_server["supported_encodings"].as<std::vector<std::string>>();
  config.ws_server_use_compression = ws_server["use_compression"].as<bool>();

  const auto& ipc = yaml_data["ipc"];
  config.ipc_spin_rate_hz = ipc["spin_rate_hz"].as<double>();
  config.ipc_topic_whitelist = ipc["topic_whitelist"].as<std::vector<std::string>>();
  config.ipc_topic_blacklist = ipc["topic_blacklist"].as<std::vector<std::string>>();
  config.ipc_service_whitelist = ipc["service_whitelist"].as<std::vector<std::string>>();
  config.ipc_service_blacklist = ipc["service_blacklist"].as<std::vector<std::string>>();

  return config;
}

void saveBridgeConfigToYaml(const WsBridgeConfig& config, const std::string& yaml_file_path) {
  YAML::Node yaml_data;

  yaml_data["ws_server"]["listening_port"] = config.ws_server_listening_port;
  yaml_data["ws_server"]["address"] = config.ws_server_address;
  yaml_data["ws_server"]["client_topic_whitelist"] = config.ws_server_client_topic_whitelist;
  yaml_data["ws_server"]["supported_encodings"] = config.ws_server_supported_encodings;
  yaml_data["ws_server"]["use_compression"] = config.ws_server_use_compression;

  yaml_data["ipc"]["spin_rate_hz"] = config.ipc_spin_rate_hz;
  yaml_data["ipc"]["topic_whitelist"] = config.ipc_topic_whitelist;
  yaml_data["ipc"]["topic_blacklist"] = config.ipc_topic_blacklist;
  yaml_data["ipc"]["service_whitelist"] = config.ipc_service_whitelist;
  yaml_data["ipc"]["service_blacklist"] = config.ipc_service_blacklist;

  std::ofstream file(yaml_file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open YAML file for writing: " + yaml_file_path);
  }

  file << yaml_data;
}

bool isMatch(const std::string& topic, const std::vector<std::string>& regex_list) {
  auto regexes = parseRegexStrings(regex_list);
  for (const auto& regex : regexes) {
    if (std::regex_match(topic, regex)) {
      return true;
    }
  }
  return false;
}

bool shouldBridgeIpcTopic(const std::string& topic, const WsBridgeConfig& config) {
  return isMatch(topic, config.ipc_topic_whitelist) && !isMatch(topic, config.ipc_service_blacklist);
}

bool shouldBridgeIpcService(const std::string& service, const WsBridgeConfig& config) {
  return isMatch(service, config.ipc_service_whitelist) && !isMatch(service, config.ipc_service_blacklist);
}

bool shouldBridgeWsTopic(const std::string& topic, const WsBridgeConfig& config) {
  return isMatch(topic, config.ws_server_client_topic_whitelist);
}

}  // namespace heph::ws_bridge
