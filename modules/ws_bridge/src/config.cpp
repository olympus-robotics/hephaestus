#include "hephaestus/ws_bridge/config.h"

namespace heph::ws_bridge {

std::vector<std::regex> ParseRegexStrings(const std::vector<std::string>& regex_string_vector) {
  std::vector<std::regex> regex_vector;
  regex_vector.reserve(regex_string_vector.size());

  for (const auto& regex_string : regex_string_vector) {
    try {
      regex_vector.push_back(
          std::regex(regex_string, std::regex_constants::ECMAScript | std::regex_constants::icase));
    } catch (const std::exception& ex) {
      heph::log(heph::ERROR, "Ignoring invalid regular expression '{}' - Error: {}", regex_string, ex.what());
    }
  }

  return regex_vector;
}

BridgeConfig LoadBridgeConfigFromYaml(const std::string& yaml_file_path) {
  std::ifstream file(yaml_file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open YAML file: " + yaml_file_path);
  }

  nlohmann::json yaml_data;
  file >> yaml_data;

  BridgeConfig config;
  const auto& ws_server = yaml_data.at("ws_server");
  config.ws_server_listening_port = ws_server.at("listening_port").get<uint16_t>();
  config.ws_server_address = ws_server.at("address").get<std::string>();
  config.ws_server_client_topic_whitelist =
      ws_server.at("client_topic_whitelist").get<std::vector<std::string>>();
  config.ws_server_supported_encodings = ws_server.at("supported_encodings").get<std::vector<std::string>>();
  config.ws_server_use_compression = ws_server.at("use_compression").get<bool>();

  const auto& ipc = yaml_data.at("ipc");
  config.ipc_spin_rate_hz = ipc.at("spin_rate_hz").get<double>();
  config.ipc_topic_whitelist = ipc.at("topic_whitelist").get<std::vector<std::string>>();
  config.ipc_service_whitelist = ipc.at("service_whitelist").get<std::vector<std::string>>();

  return config;
}

void SaveBridgeConfigToYaml(const BridgeConfig& config, const std::string& yaml_file_path) {
  nlohmann::json yaml_data;

  yaml_data["ws_server"]["listening_port"] = config.ws_server_listening_port;
  yaml_data["ws_server"]["address"] = config.ws_server_address;
  yaml_data["ws_server"]["client_topic_whitelist"] = config.ws_server_client_topic_whitelist;
  yaml_data["ws_server"]["supported_encodings"] = config.ws_server_supported_encodings;
  yaml_data["ws_server"]["use_compression"] = config.ws_server_use_compression;

  yaml_data["ipc"]["spin_rate_hz"] = config.ipc_spin_rate_hz;
  yaml_data["ipc"]["topic_whitelist"] = config.ipc_topic_whitelist;
  yaml_data["ipc"]["service_whitelist"] = config.ipc_service_whitelist;

  std::ofstream file(yaml_file_path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open YAML file for writing: " + yaml_file_path);
  }

  file << yaml_data.dump(4);  // Pretty print with 4 spaces indentation
}

}  // namespace heph::ws_bridge