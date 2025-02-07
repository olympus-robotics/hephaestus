#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws_bridge::tests {

TEST(ConfigParsing, BridgeConfigYamlLoadAndSave) {
  WsBridgeConfig config;
  config.ws_server_listening_port = 8080;
  config.ws_server_address = "127.0.0.1";
  config.ws_server_client_topic_whitelist = { "^topic1.*", ".*topic2$" };
  config.ws_server_supported_encodings = { "json", "protobuf" };
  config.ws_server_use_compression = true;
  config.ipc_spin_rate_hz = 10.0;
  config.ipc_topic_whitelist = { "topic1", "topic2" };
  config.ipc_topic_blacklist = { "blacklist1", "blacklist2" };
  config.ipc_service_whitelist = { "service1", "service2" };
  config.ipc_service_blacklist = { "blacklist_service1", "blacklist_service2" };

  saveBridgeConfigToYaml(config, "/tmp/test_config.yaml");

  WsBridgeConfig loaded_config = loadBridgeConfigFromYaml("/tmp/test_config.yaml");

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
  ASSERT_EQ(config.ipc_topic_blacklist.size(), loaded_config.ipc_topic_blacklist.size());
  for (size_t i = 0; i < config.ipc_topic_blacklist.size(); ++i) {
    EXPECT_EQ(config.ipc_topic_blacklist[i], loaded_config.ipc_topic_blacklist[i]);
  }
  ASSERT_EQ(config.ipc_service_whitelist.size(), loaded_config.ipc_service_whitelist.size());
  for (size_t i = 0; i < config.ipc_service_whitelist.size(); ++i) {
    EXPECT_EQ(config.ipc_service_whitelist[i], loaded_config.ipc_service_whitelist[i]);
  }
  ASSERT_EQ(config.ipc_service_blacklist.size(), loaded_config.ipc_service_blacklist.size());
  for (size_t i = 0; i < config.ipc_service_blacklist.size(); ++i) {
    EXPECT_EQ(config.ipc_service_blacklist[i], loaded_config.ipc_service_blacklist[i]);
  }
}

TEST(ConfigParsing, parseRegexStrings) {
  std::vector<std::string> regex_strings = { "^test.*", ".*example$" };
  std::vector<std::regex> regexes = parseRegexStrings(regex_strings);

  ASSERT_EQ(regexes.size(), 2);
  EXPECT_TRUE(std::regex_match("test123", regexes[0]));
  EXPECT_TRUE(std::regex_match("myexample", regexes[1]));
}

TEST(ConfigParsing, isMatch) {
  std::vector<std::string> regex_list = { "^topic1.*", ".*topic2$" };

  EXPECT_TRUE(isMatch("topic1_test", regex_list));
  EXPECT_TRUE(isMatch("test_topic2", regex_list));
  EXPECT_FALSE(isMatch("random_topic", regex_list));
}

TEST(ConfigParsing, shouldBridgeIpcTopic) {
  WsBridgeConfig config;
  config.ipc_topic_whitelist = { "^topic1.*", ".*topic2$" };
  config.ipc_topic_blacklist = { "blacklist1", "blacklist2" };

  EXPECT_TRUE(shouldBridgeIpcTopic("topic1_test", config));
  EXPECT_TRUE(shouldBridgeIpcTopic("test_topic2", config));
  EXPECT_FALSE(shouldBridgeIpcTopic("blacklist1", config));
  EXPECT_FALSE(shouldBridgeIpcTopic("random_topic", config));
}

TEST(ConfigParsing, shouldBridgeIpcService) {
  WsBridgeConfig config;
  config.ipc_service_whitelist = { "^service1.*", ".*service2$" };
  config.ipc_service_blacklist = { "blacklist_service1", "blacklist_service2" };

  EXPECT_TRUE(shouldBridgeIpcService("service1_test", config));
  EXPECT_TRUE(shouldBridgeIpcService("test_service2", config));
  EXPECT_FALSE(shouldBridgeIpcService("blacklist_service1", config));
  EXPECT_FALSE(shouldBridgeIpcService("random_service", config));
}

TEST(ConfigParsing, shouldBridgeWsTopic) {
  WsBridgeConfig config;
  config.ws_server_client_topic_whitelist = { "^topic1.*", ".*topic2$" };

  EXPECT_TRUE(shouldBridgeWsTopic("topic1_test", config));
  EXPECT_TRUE(shouldBridgeWsTopic("test_topic2", config));
  EXPECT_FALSE(shouldBridgeWsTopic("random_topic", config));
}

}  // namespace heph::ws_bridge::tests
