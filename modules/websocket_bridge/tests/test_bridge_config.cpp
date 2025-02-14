#include <gtest/gtest.h>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws_bridge {

TEST(ParseRegexStringsTest, ValidRegex) {
  std::vector<std::string> regex_strings = { ".*", "^test$", "a+b*" };
  auto regexes = parseRegexStrings(regex_strings);
  EXPECT_EQ(regexes.size(), regex_strings.size());
}

TEST(ParseRegexStringsTest, InvalidRegex) {
  std::vector<std::string> regex_strings = { ".*", "(", "a+b*" };
  auto regexes = parseRegexStrings(regex_strings);
  EXPECT_EQ(regexes.size(), regex_strings.size() - 1);  // One invalid regex
}

TEST(IsMatchTest, RegexMatch) {
  std::vector<std::regex> regex_list = { std::regex("^test$") };
  EXPECT_TRUE(isMatch("test", regex_list));
  EXPECT_FALSE(isMatch("not_test", regex_list));
}

TEST(IsMatchTest, StringMatch) {
  std::vector<std::string> regex_strings = { "^test$" };
  EXPECT_TRUE(isMatch("test", regex_strings));
  EXPECT_FALSE(isMatch("not_test", regex_strings));
}

TEST(ShouldBridgeIpcTopicTest, WhitelistAndBlacklist) {
  WsBridgeConfig config;
  config.ipc_topic_whitelist = { ".*" };
  config.ipc_topic_blacklist = { "^exclude$" };
  EXPECT_TRUE(shouldBridgeIpcTopic("include", config));
  EXPECT_FALSE(shouldBridgeIpcTopic("exclude", config));
}

TEST(ShouldBridgeIpcTopicTest, WhitelistOnly) {
  WsBridgeConfig config;
  config.ipc_topic_whitelist = { ".*" };
  EXPECT_TRUE(shouldBridgeIpcTopic("include", config));
  EXPECT_TRUE(shouldBridgeIpcTopic("exclude", config));
}

TEST(ShouldBridgeIpcServiceTest, WhitelistAndBlacklist) {
  WsBridgeConfig config;
  config.ipc_service_whitelist = { ".*" };
  config.ipc_service_blacklist = { "^exclude$" };
  EXPECT_TRUE(shouldBridgeIpcService("include", config));
  EXPECT_FALSE(shouldBridgeIpcService("exclude", config));
}

TEST(ShouldBridgeIpcServiceTest, WhitelistOnly) {
  WsBridgeConfig config;
  config.ipc_service_whitelist = { ".*" };
  EXPECT_TRUE(shouldBridgeIpcService("include", config));
  EXPECT_TRUE(shouldBridgeIpcService("exclude", config));
}

TEST(ShouldBridgeWsTopicTest, MatchWhitelist) {
  WsBridgeConfig config;
  config.ws_server_config.clientTopicWhitelistPatterns = parseRegexStrings({ ".*incl.*" });
  EXPECT_TRUE(shouldBridgeWsTopic("include", config));
  EXPECT_FALSE(shouldBridgeWsTopic("exclude", config));
}

TEST(LoadBridgeConfigFromYamlTest, SaveDefaultAndLoad) {
  WsBridgeConfig original_config;
  saveBridgeConfigToYaml(original_config, "/tmp/default.yaml");

  auto config = loadBridgeConfigFromYaml("/tmp/default.yaml");

  EXPECT_EQ(original_config.ws_server_listening_port, config.ws_server_listening_port);
  EXPECT_EQ(original_config.ws_server_address, config.ws_server_address);
  EXPECT_EQ(original_config.zenoh_config.id, config.zenoh_config.id);
  EXPECT_EQ(original_config.zenoh_config.router, config.zenoh_config.router);
  EXPECT_EQ(original_config.ipc_topic_whitelist, config.ipc_topic_whitelist);
  EXPECT_EQ(original_config.ipc_topic_blacklist, config.ipc_topic_blacklist);
  EXPECT_EQ(original_config.ipc_service_whitelist, config.ipc_service_whitelist);
  EXPECT_EQ(original_config.ipc_service_blacklist, config.ipc_service_blacklist);
  EXPECT_EQ(original_config.ws_server_config.clientTopicWhitelistPatterns.size(),
            config.ws_server_config.clientTopicWhitelistPatterns.size());
  EXPECT_EQ(original_config.ws_server_config.useCompression, config.ws_server_config.useCompression);
  EXPECT_EQ(original_config.ws_server_config.sendBufferLimitBytes,
            config.ws_server_config.sendBufferLimitBytes);
  EXPECT_EQ(original_config.ws_server_config.useTls, config.ws_server_config.useTls);
  EXPECT_EQ(original_config.ws_server_config.certfile, config.ws_server_config.certfile);
  EXPECT_EQ(original_config.ws_server_config.keyfile, config.ws_server_config.keyfile);
  EXPECT_EQ(original_config.ws_server_config.sessionId, config.ws_server_config.sessionId);
  EXPECT_EQ(original_config.ws_server_config.capabilities.size(),
            config.ws_server_config.capabilities.size());
  EXPECT_EQ(original_config.ws_server_config.capabilities[0], config.ws_server_config.capabilities[0]);
  EXPECT_EQ(original_config.zenoh_config.use_binary_name_as_session_id,
            config.zenoh_config.use_binary_name_as_session_id);
  EXPECT_EQ(original_config.zenoh_config.id, config.zenoh_config.id);
  EXPECT_EQ(original_config.zenoh_config.enable_shared_memory, config.zenoh_config.enable_shared_memory);
  EXPECT_EQ(original_config.zenoh_config.mode, config.zenoh_config.mode);
  EXPECT_EQ(original_config.zenoh_config.router, config.zenoh_config.router);
  EXPECT_EQ(original_config.zenoh_config.cache_size, config.zenoh_config.cache_size);
  EXPECT_EQ(original_config.zenoh_config.qos, config.zenoh_config.qos);
  EXPECT_EQ(original_config.zenoh_config.real_time, config.zenoh_config.real_time);
  EXPECT_EQ(original_config.zenoh_config.protocol, config.zenoh_config.protocol);
  EXPECT_EQ(original_config.zenoh_config.multicast_scouting_enabled,
            config.zenoh_config.multicast_scouting_enabled);
  EXPECT_EQ(original_config.zenoh_config.multicast_scouting_interface,
            config.zenoh_config.multicast_scouting_interface);
}

TEST(LoadBridgeConfigFromYamlTest, LoadInvalidYaml) {
  std::string yaml_content = R"(
  invalid_yaml_content
  )";
  std::ofstream yaml_file("/tmp/invalid_test_config.yaml");
  yaml_file << yaml_content;
  yaml_file.close();

  EXPECT_THROW(loadBridgeConfigFromYaml("/tmp/invalid_test_config.yaml"), std::runtime_error);
}

TEST(SaveBridgeConfigToYamlTest, SaveInvalidPath) {
  WsBridgeConfig config;
  EXPECT_THROW(saveBridgeConfigToYaml(config, "/invalid_path/saved_config.yaml"), std::runtime_error);
}

TEST(LoadBridgeConfigFromYamlTest, LoadInvalidPath) {
  EXPECT_THROW(loadBridgeConfigFromYaml("/invalid_path/config.yaml"), std::runtime_error);
}

}  // namespace heph::ws_bridge