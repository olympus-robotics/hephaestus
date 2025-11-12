//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <exception>
#include <fstream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class MyEnvironment : public ::testing::Environment {
public:
  ~MyEnvironment() override = default;

  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>(heph::INFO));
  }

private:
  error_handling::PanicAsExceptionScope panic_scope_;
};

// NOLINTNEXTLINE
auto* const my_env = dynamic_cast<MyEnvironment*>(AddGlobalTestEnvironment(new MyEnvironment{}));

TEST(BridgeConfigTest, ValidRegex) {
  const std::vector<std::string> regex_strings = { ".*", "^test$", "a+b*" };
  const auto regexes = parseRegexStrings(regex_strings);
  EXPECT_EQ(regexes.size(), regex_strings.size());
}

TEST(BridgeConfigTest, InvalidRegex) {
  const std::vector<std::string> regex_strings = { ".*", "(", "a+b*" };
  const auto regexes = parseRegexStrings(regex_strings);
  EXPECT_EQ(regexes.size(), regex_strings.size() - 1);  // One invalid regex
}

TEST(BridgeConfigTest, RegexMatch) {
  const std::vector<std::regex> regex_list = { std::regex("^test$") };
  EXPECT_TRUE(isMatch("test", regex_list));
  EXPECT_FALSE(isMatch("not_test", regex_list));
}

TEST(BridgeConfigTest, StringMatch) {
  const std::vector<std::string> regex_strings = { "^test$" };
  EXPECT_TRUE(isMatch("test", regex_strings));
  EXPECT_FALSE(isMatch("not_test", regex_strings));
}

TEST(BridgeConfigTest, WhitelistAndBlacklistA) {
  WebsocketBridgeConfig config;
  config.ipc_topic_whitelist = { ".*" };
  config.ipc_topic_blacklist = { "^exclude$" };
  EXPECT_TRUE(shouldBridgeIpcTopic("include", config));
  EXPECT_FALSE(shouldBridgeIpcTopic("exclude", config));
}

TEST(BridgeConfigTest, WhitelistOnlyA) {
  WebsocketBridgeConfig config;
  config.ipc_topic_whitelist = { ".*" };
  EXPECT_TRUE(shouldBridgeIpcTopic("include", config));
  EXPECT_TRUE(shouldBridgeIpcTopic("exclude", config));
}

TEST(BridgeConfigTest, WhitelistAndBlacklistB) {
  WebsocketBridgeConfig config;
  config.ipc_service_whitelist = { ".*" };
  config.ipc_service_blacklist = { "^exclude$" };
  EXPECT_TRUE(shouldBridgeIpcService("include", config));
  EXPECT_FALSE(shouldBridgeIpcService("exclude", config));
}

TEST(BridgeConfigTest, WhitelistOnlyB) {
  WebsocketBridgeConfig config;
  config.ipc_service_whitelist = { ".*" };
  EXPECT_TRUE(shouldBridgeIpcService("include", config));
  EXPECT_TRUE(shouldBridgeIpcService("exclude", config));
}

TEST(BridgeConfigTest, MatchWhitelist) {
  WebsocketBridgeConfig config;
  config.ws_server_config.clientTopicWhitelistPatterns = parseRegexStrings({ ".*incl.*" });
  EXPECT_TRUE(shouldBridgeWsTopic("include", config));
  EXPECT_FALSE(shouldBridgeWsTopic("exclude", config));
}

TEST(BridgeConfigTest, SaveDefaultAndLoad) {
  WebsocketBridgeConfig original_config;
  try {
    saveBridgeConfigToYaml(original_config, "/tmp/default.yaml");
  } catch (const std::exception& e) {
    FAIL() << "Failed to save default config: " << e.what();
  }

  WebsocketBridgeConfig config;
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

TEST(BridgeConfigTest, LoadInvalidYaml) {
  const std::string yaml_content = R"(
  invalid_yaml_content
  )";
  std::ofstream yaml_file("/tmp/invalid_test_config.yaml");
  yaml_file << yaml_content;
  yaml_file.close();

  EXPECT_THROW(auto config = loadBridgeConfigFromYaml("/tmp/invalid_test_config.yaml"),
               error_handling::PanicException);
}

TEST(BridgeConfigTest, SaveInvalidPath) {
  const WebsocketBridgeConfig config;
  EXPECT_THROW(saveBridgeConfigToYaml(config, "/invalid_path/saved_config.yaml"),
               error_handling::PanicException);
}

TEST(BridgeConfigTest, LoadInvalidPath) {
  EXPECT_THROW(const auto config = loadBridgeConfigFromYaml("/invalid_path/config.yaml"),
               error_handling::PanicException);
}

}  // namespace heph::ws
