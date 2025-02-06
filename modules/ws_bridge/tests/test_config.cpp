#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include "hephaestus/ws_bridge/config.h"

namespace heph::ws_bridge::tests {

TEST(ConfigParsing, BridgeConfigYamlLoadAndSave) {
  BridgeConfig config;
  config.ws_server_listening_port = 8080;
  config.ws_server_address = "127.0.0.1";
  config.ws_server_client_topic_whitelist = { "^topic1.*", ".*topic2$" };
  config.ws_server_supported_encodings = { "json", "protobuf" };
  config.ws_server_use_compression = true;
  config.ipc_spin_rate_hz = 10.0;
  config.ipc_topic_whitelist = { "topic1", "topic2" };
  config.ipc_service_whitelist = { "service1", "service2" };

  SaveBridgeConfigToYaml(config, "/tmp/test_config.yaml");

  BridgeConfig loaded_config = LoadBridgeConfigFromYaml("/tmp/test_config.yaml");

  EXPECT_EQ(config.ws_server_listening_port, loaded_config.ws_server_listening_port);
  EXPECT_EQ(config.ws_server_address, loaded_config.ws_server_address);
  ASSERT_EQ(config.ws_server_client_topic_whitelist.size(),
            loaded_config.ws_server_client_topic_whitelist.size());
  for (size_t i = 0; i < config.ws_server_client_topic_whitelist.size(); ++i) {
    EXPECT_EQ(config.ws_server_client_topic_whitelist[i], loaded_config.ws_server_client_topic_whitelist[i]);
  }
  ASSERT_EQ(config.ws_server_supported_encodings.size(), loaded_config.ws_server_supported_encodings.size());
  for (size_t i = 0; i < config.ws_server_supported_encodings.size(); ++i) {
    EXPECT_EQ(config.ws_server_supported_encodings[i], loaded_config.ws_server_supported_encodings[i]);
  }
  EXPECT_EQ(config.ws_server_use_compression, loaded_config.ws_server_use_compression);
  EXPECT_EQ(config.ipc_spin_rate_hz, loaded_config.ipc_spin_rate_hz);
  ASSERT_EQ(config.ipc_topic_whitelist.size(), loaded_config.ipc_topic_whitelist.size());
  for (size_t i = 0; i < config.ipc_topic_whitelist.size(); ++i) {
    EXPECT_EQ(config.ipc_topic_whitelist[i], loaded_config.ipc_topic_whitelist[i]);
  }
  ASSERT_EQ(config.ipc_service_whitelist.size(), loaded_config.ipc_service_whitelist.size());
  for (size_t i = 0; i < config.ipc_service_whitelist.size(); ++i) {
    EXPECT_EQ(config.ipc_service_whitelist[i], loaded_config.ipc_service_whitelist[i]);
  }
}

TEST(ConfigParsing, ParseRegexStrings) {
  std::vector<std::string> regex_strings = { "^test.*", ".*example$" };
  std::vector<std::regex> regexes = ParseRegexStrings(regex_strings);

  ASSERT_EQ(regexes.size(), 2);
  EXPECT_TRUE(std::regex_match("test123", regexes[0]));
  EXPECT_TRUE(std::regex_match("myexample", regexes[1]));
}

}  // namespace heph::ws_bridge::tests