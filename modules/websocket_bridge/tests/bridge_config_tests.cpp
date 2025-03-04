//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws {

class BridgeConfigTest : public ::testing::Test {
protected:
  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
  }

  void TearDown() override {
  }
};

TEST_F(BridgeConfigTest, ValidRegex) {
  std::vector<std::string> regex_strings = { ".*", "^test$", "a+b*" };
  auto regexes = parseRegexStrings(regex_strings);
  EXPECT_EQ(regexes.size(), regex_strings.size());
}

TEST_F(BridgeConfigTest, InvalidRegex) {
  std::vector<std::string> regex_strings = { ".*", "(", "a+b*" };
  auto regexes = parseRegexStrings(regex_strings);
  EXPECT_EQ(regexes.size(), regex_strings.size() - 1);  // One invalid regex
}

TEST_F(BridgeConfigTest, RegexMatch) {
  std::vector<std::regex> regex_list = { std::regex("^test$") };
  EXPECT_TRUE(isMatch("test", regex_list));
  EXPECT_FALSE(isMatch("not_test", regex_list));
}

TEST_F(BridgeConfigTest, StringMatch) {
  std::vector<std::string> regex_strings = { "^test$" };
  EXPECT_TRUE(isMatch("test", regex_strings));
  EXPECT_FALSE(isMatch("not_test", regex_strings));
}

TEST_F(BridgeConfigTest, WhitelistAndBlacklistA) {
  WsBridgeConfig config;
  config.ipc_topic_whitelist = { ".*" };
  config.ipc_topic_blacklist = { "^exclude$" };
  EXPECT_TRUE(shouldBridgeIpcTopic("include", config));
  EXPECT_FALSE(shouldBridgeIpcTopic("exclude", config));
}

TEST_F(BridgeConfigTest, WhitelistOnlyA) {
  WsBridgeConfig config;
  config.ipc_topic_whitelist = { ".*" };
  EXPECT_TRUE(shouldBridgeIpcTopic("include", config));
  EXPECT_TRUE(shouldBridgeIpcTopic("exclude", config));
}

TEST_F(BridgeConfigTest, WhitelistAndBlacklistB) {
  WsBridgeConfig config;
  config.ipc_service_whitelist = { ".*" };
  config.ipc_service_blacklist = { "^exclude$" };
  EXPECT_TRUE(shouldBridgeIpcService("include", config));
  EXPECT_FALSE(shouldBridgeIpcService("exclude", config));
}

TEST_F(BridgeConfigTest, WhitelistOnlyB) {
  WsBridgeConfig config;
  config.ipc_service_whitelist = { ".*" };
  EXPECT_TRUE(shouldBridgeIpcService("include", config));
  EXPECT_TRUE(shouldBridgeIpcService("exclude", config));
}

TEST_F(BridgeConfigTest, MatchWhitelist) {
  WsBridgeConfig config;
  config.ws_server_config.clientTopicWhitelistPatterns = parseRegexStrings({ ".*incl.*" });
  EXPECT_TRUE(shouldBridgeWsTopic("include", config));
  EXPECT_FALSE(shouldBridgeWsTopic("exclude", config));
}

TEST_F(BridgeConfigTest, SaveDefaultAndLoad) {
  WsBridgeConfig original_config;
  try {
    saveBridgeConfigToYaml(original_config, "/tmp/default.yaml");
  } catch (const std::exception& e) {
    FAIL() << "Failed to save default config: " << e.what();
  }

  WsBridgeConfig config;
  try {
    config = loadBridgeConfigFromYaml("/tmp/default.yaml");
  } catch (const std::exception& e) {
    FAIL() << "Failed to load config: " << e.what();
  }

  EXPECT_EQ(original_config.ws_server_port, config.ws_server_port);
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
  EXPECT_EQ(original_config.ws_server_config.numWorkerThreads, config.ws_server_config.numWorkerThreads);
  EXPECT_EQ(original_config.ws_server_config.capabilities.size(),
            config.ws_server_config.capabilities.size());
  EXPECT_EQ(original_config.ws_server_config.capabilities[0], config.ws_server_config.capabilities[0]);
  EXPECT_EQ(original_config.zenoh_config.use_binary_name_as_session_id,
            config.zenoh_config.use_binary_name_as_session_id);
  EXPECT_EQ(original_config.zenoh_config.id, config.zenoh_config.id);
  EXPECT_EQ(original_config.zenoh_config.enable_shared_memory, config.zenoh_config.enable_shared_memory);
  EXPECT_EQ(original_config.zenoh_config.mode, config.zenoh_config.mode);
  EXPECT_EQ(original_config.zenoh_config.router, config.zenoh_config.router);
  EXPECT_EQ(original_config.zenoh_config.qos, config.zenoh_config.qos);
  EXPECT_EQ(original_config.zenoh_config.real_time, config.zenoh_config.real_time);
  EXPECT_EQ(original_config.zenoh_config.protocol, config.zenoh_config.protocol);
  EXPECT_EQ(original_config.zenoh_config.multicast_scouting_enabled,
            config.zenoh_config.multicast_scouting_enabled);
  EXPECT_EQ(original_config.zenoh_config.multicast_scouting_interface,
            config.zenoh_config.multicast_scouting_interface);
}

TEST_F(BridgeConfigTest, LoadInvalidYaml) {
  std::string yaml_content = R"(
  invalid_yaml_content
  )";
  std::ofstream yaml_file("/tmp/invalid_test_config.yaml");
  yaml_file << yaml_content;
  yaml_file.close();

  EXPECT_THROW(auto config = loadBridgeConfigFromYaml("/tmp/invalid_test_config.yaml"), std::runtime_error);
}

TEST_F(BridgeConfigTest, SaveInvalidPath) {
  WsBridgeConfig config;
  EXPECT_THROW(saveBridgeConfigToYaml(config, "/invalid_path/saved_config.yaml"), std::runtime_error);
}

TEST_F(BridgeConfigTest, LoadInvalidPath) {
  EXPECT_THROW(auto config = loadBridgeConfigFromYaml("/invalid_path/config.yaml"), std::runtime_error);
}

}  // namespace heph::ws
