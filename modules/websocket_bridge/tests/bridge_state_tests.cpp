//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/websocket_bridge/bridge_state.h"
#include "hephaestus/websocket_bridge/utils/ws_protocol.h"

namespace heph::ws::tests {

class WebsocketBridgeStateTest : public ::testing::Test {
protected:
  void SetUp() override {
    heph::telemetry::makeAndRegisterLogSink<heph::telemetry::AbslLogSink>();

    client_handle1_shared_ptr = std::make_shared<int>(1);
    client_handle2_shared_ptr = std::make_shared<int>(2);
    client_handle1 = client_handle1_shared_ptr;
    client_handle2 = client_handle2_shared_ptr;

    ASSERT_FALSE(client_handle1.expired());
    ASSERT_FALSE(client_handle2.expired());
  }

  void TearDown() override {
  }

  // NOLINTBEGIN
  // Note: The following variables are perfectly ok to be accessible from within the test.
  WebsocketBridgeState state;
  WsChannelId channel_id1 = 1;
  WsChannelId channel_id2 = 2;
  std::string topic1 = "topic1";
  std::string topic2 = "topic2";

  std::shared_ptr<int> client_handle1_shared_ptr;
  std::shared_ptr<int> client_handle2_shared_ptr;

  WsClientHandle client_handle1;
  WsClientHandle client_handle2;
  std::string client_name1 = "client1";
  std::string client_name2 = "client2";
  // NOLINTEND
};

TEST_F(WebsocketBridgeStateTest, AddAndGetIpcTopicForWsChannel) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_EQ(state.getIpcTopicForWsChannel(channel_id1), topic1);
}

TEST_F(WebsocketBridgeStateTest, GetIpcTopicForWsChannelNotFound) {
  EXPECT_EQ(state.getIpcTopicForWsChannel(channel_id1), "");
}

TEST_F(WebsocketBridgeStateTest, AddAndGetWsChannelForIpcTopic) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_EQ(state.getWsChannelForIpcTopic(topic1), channel_id1);
}

TEST_F(WebsocketBridgeStateTest, GetWsChannelForIpcTopicNotFound) {
  EXPECT_EQ(state.getWsChannelForIpcTopic(topic1), WsChannelId{});
}

TEST_F(WebsocketBridgeStateTest, RemoveWsChannelToIpcTopicMapping) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.removeWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_EQ(state.getIpcTopicForWsChannel(channel_id1), "");
  EXPECT_EQ(state.getWsChannelForIpcTopic(topic1), WsChannelId{});
}

TEST_F(WebsocketBridgeStateTest, HasWsChannelMapping) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_TRUE(state.hasWsChannelMapping(channel_id1));
  EXPECT_FALSE(state.hasWsChannelMapping(channel_id2));
}

TEST_F(WebsocketBridgeStateTest, HasIpcTopicMapping) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_TRUE(state.hasIpcTopicMapping(topic1));
  EXPECT_FALSE(state.hasIpcTopicMapping(topic2));
}

TEST_F(WebsocketBridgeStateTest, TopicChannelMappingToString) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_NE(state.topicChannelMappingToString().find("[1]"), std::string::npos);
  EXPECT_NE(state.topicChannelMappingToString().find("client1"), std::string::npos);
  EXPECT_NE(state.topicChannelMappingToString().find("topic1"), std::string::npos);
}

TEST_F(WebsocketBridgeStateTest, AddAndGetClientsForWsChannel) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  auto clients = state.getClientsForWsChannel(channel_id1);
  ASSERT_TRUE(clients.has_value());
  EXPECT_EQ(clients->size(), 1);                      // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(clients->begin()->second, client_name1);  // NOLINT(bugprone-unchecked-optional-access)
}

TEST_F(WebsocketBridgeStateTest, GetClientsForWsChannelNotFound) {
  auto clients = state.getClientsForWsChannel(channel_id1);
  EXPECT_FALSE(clients.has_value());
}

TEST_F(WebsocketBridgeStateTest, RemoveWsChannelToClientMapping) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  state.removeWsChannelToClientMapping(channel_id1);
  auto clients = state.getClientsForWsChannel(channel_id1);
  EXPECT_FALSE(clients.has_value());
}

TEST_F(WebsocketBridgeStateTest, RemoveSpecificClientFromWsChannel) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  state.addWsChannelToClientMapping(channel_id1, client_handle2, client_name2);
  state.removeWsChannelToClientMapping(channel_id1, client_handle1);
  auto clients = state.getClientsForWsChannel(channel_id1);
  ASSERT_TRUE(clients.has_value());
  EXPECT_EQ(clients->size(), 1);                      // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(clients->begin()->second, client_name2);  // NOLINT(bugprone-unchecked-optional-access)
}

TEST_F(WebsocketBridgeStateTest, HasWsChannelWithClients) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_TRUE(state.hasWsChannelWithClients(channel_id1));
  EXPECT_FALSE(state.hasWsChannelWithClients(channel_id2));
}

TEST_F(WebsocketBridgeStateTest, ToString) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_NE(state.toString().find("[1]"), std::string::npos);
  EXPECT_NE(state.toString().find("client1"), std::string::npos);
}

TEST_F(WebsocketBridgeStateTest, AddAndRetrieveServiceMapping) {
  const WsServiceId service_id = 101;
  const std::string service_name = "test_service";
  state.addWsServiceToIpcServiceMapping(service_id, service_name);

  EXPECT_EQ(state.getIpcServiceForWsService(service_id), service_name);
  EXPECT_EQ(state.getWsServiceForIpcService(service_name), service_id);
}

TEST_F(WebsocketBridgeStateTest, RemoveServiceMapping) {
  const WsServiceId service_id = 202;
  const std::string service_name = "removable_service";
  state.addWsServiceToIpcServiceMapping(service_id, service_name);
  state.removeWsServiceToIpcServiceMapping(service_id, service_name);

  EXPECT_FALSE(state.hasWsServiceMapping(service_id));
  EXPECT_FALSE(state.hasIpcServiceMapping(service_name));
}

TEST_F(WebsocketBridgeStateTest, ServiceMappingToString) {
  const WsServiceId service_id = 303;
  const std::string service_name = "string_service";
  state.addWsServiceToIpcServiceMapping(service_id, service_name);

  auto mapping_str = state.servicMappingToString();
  EXPECT_NE(mapping_str.find(service_name), std::string::npos);
}

// Additional tests for WebsocketBridgeState

// Client channel to topic mappings
TEST_F(WebsocketBridgeStateTest, AddAndGetTopicForClientChannel) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_EQ(state.getTopicForClientChannel(client_channel_id), topic1);
}

TEST_F(WebsocketBridgeStateTest, GetClientChannelsForTopic) {
  const WsClientChannelId client_channel_id1 = 10001;
  const WsClientChannelId client_channel_id2 = 10002;
  state.addClientChannelToTopicMapping(client_channel_id1, topic1);
  state.addClientChannelToTopicMapping(client_channel_id2, topic1);

  auto client_channels = state.getClientChannelsForTopic(topic1);
  EXPECT_EQ(client_channels.size(), 2);
  EXPECT_TRUE(client_channels.contains(client_channel_id1));
  EXPECT_TRUE(client_channels.contains(client_channel_id2));
}

TEST_F(WebsocketBridgeStateTest, GetTopicForClientChannelNotFound) {
  const WsClientChannelId client_channel_id = 10001;
  EXPECT_EQ(state.getTopicForClientChannel(client_channel_id), "");
}

TEST_F(WebsocketBridgeStateTest, GetClientChannelsForTopicNotFound) {
  auto client_channels = state.getClientChannelsForTopic(topic1);
  EXPECT_TRUE(client_channels.empty());
}

TEST_F(WebsocketBridgeStateTest, RemoveClientChannelToTopicMapping) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_TRUE(state.hasClientChannelMapping(client_channel_id));

  state.removeClientChannelToTopicMapping(client_channel_id);
  EXPECT_FALSE(state.hasClientChannelMapping(client_channel_id));
  EXPECT_FALSE(state.hasTopicToClientChannelMapping(topic1));
}

TEST_F(WebsocketBridgeStateTest, HasClientChannelMapping) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_TRUE(state.hasClientChannelMapping(client_channel_id));
  EXPECT_FALSE(state.hasClientChannelMapping(10002));
}

TEST_F(WebsocketBridgeStateTest, HasWsChannelWithClientsNoClientsInMap) {
  state.addWsChannelToIpcTopicMapping(channel_id2, topic2);
  EXPECT_FALSE(state.hasWsChannelWithClients(channel_id2));
}

TEST_F(WebsocketBridgeStateTest, HasWsChannelWithClientsExpiredHandle) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  client_handle1_shared_ptr.reset();
  // NOTE: The handle is expired, but without a call to the
  // cleanup function, the entry still persists.
  EXPECT_TRUE(state.hasWsChannelWithClients(channel_id1));
}

// Test for checkConsistency()
TEST_F(WebsocketBridgeStateTest, CheckConsistencyValidState) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_TRUE(state.checkConsistency());
}

// Tests for WS Service Call ID <-> WS Clients
TEST_F(WebsocketBridgeStateTest, HasCallIdToClientMapping) {
  static constexpr uint32_t CALL_ID = 5000;
  EXPECT_FALSE(state.hasCallIdToClientMapping(CALL_ID));
  state.addCallIdToClientMapping(CALL_ID, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(CALL_ID));
}

TEST_F(WebsocketBridgeStateTest, AddAndGetClientForCallId) {
  static constexpr uint32_t CALL_ID = 5001;
  state.addCallIdToClientMapping(CALL_ID, client_handle1, client_name1);

  auto client = state.getClientForCallId(CALL_ID);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);  // NOLINT(bugprone-unchecked-optional-access)
}

TEST_F(WebsocketBridgeStateTest, GetClientForCallIdNotFound) {
  static constexpr uint32_t CALL_ID = 5002;
  auto client = state.getClientForCallId(CALL_ID);
  EXPECT_FALSE(client.has_value());
}

TEST_F(WebsocketBridgeStateTest, RemoveCallIdToClientMapping) {
  static constexpr uint32_t CALL_ID = 5003;
  state.addCallIdToClientMapping(CALL_ID, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(CALL_ID));

  state.removeCallIdToClientMapping(CALL_ID);
  EXPECT_FALSE(state.hasCallIdToClientMapping(CALL_ID));
}

TEST_F(WebsocketBridgeStateTest, CallIdToClientMappingToString) {
  static constexpr uint32_t CALL_ID = 5004;
  state.addCallIdToClientMapping(CALL_ID, client_handle1, client_name1);

  auto mapping_str = state.callIdToClientMappingToString();
  EXPECT_NE(mapping_str.find(std::to_string(CALL_ID)), std::string::npos);
  EXPECT_NE(mapping_str.find(client_name1), std::string::npos);
}

TEST_F(WebsocketBridgeStateTest, CleanUpCallIdToClientMappingExpiredHandle) {
  static constexpr uint32_t CALL_ID = 5005;
  static constexpr uint32_t CALL_ID_2 = 5006;
  state.addCallIdToClientMapping(CALL_ID, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(CALL_ID));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Add another mapping to trigger cleanup
  state.addCallIdToClientMapping(CALL_ID_2, client_handle2, client_name2);

  // Verify the expired handle was cleaned up
  EXPECT_FALSE(state.hasCallIdToClientMapping(CALL_ID));
  EXPECT_TRUE(state.hasCallIdToClientMapping(CALL_ID_2));
}

// Test for clientChannelMappingToString
TEST_F(WebsocketBridgeStateTest, ClientChannelMappingToString) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);

  auto mapping_str = state.clientChannelMappingToString();
  EXPECT_NE(mapping_str.find(topic1), std::string::npos);
  EXPECT_NE(mapping_str.find(std::to_string(client_channel_id)), std::string::npos);
  EXPECT_NE(mapping_str.find(client_name1), std::string::npos);
}

// Tests for WS Client Channels <-> WS Clients
TEST_F(WebsocketBridgeStateTest, HasClientForClientChannel) {
  const WsClientChannelId client_channel_id = 10001;
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));

  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));
}

TEST_F(WebsocketBridgeStateTest, AddAndGetClientForClientChannel) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);

  auto client = state.getClientForClientChannel(client_channel_id);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);  // NOLINT(bugprone-unchecked-optional-access)
}

TEST_F(WebsocketBridgeStateTest, GetClientForClientChannelNotFound) {
  const WsClientChannelId client_channel_id = 10001;
  auto client = state.getClientForClientChannel(client_channel_id);
  EXPECT_FALSE(client.has_value());
}

TEST_F(WebsocketBridgeStateTest, RemoveClientChannelToClientMapping) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  state.removeClientChannelToClientMapping(client_channel_id);
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

TEST_F(WebsocketBridgeStateTest, CleanUpClientChannelToClientMappingExpiredHandle) {
  const WsClientChannelId client_channel_id = 10001;
  const WsClientChannelId client_channel_id_2 = 10002;

  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Add another mapping to trigger cleanup
  state.addClientChannelToClientMapping(client_channel_id_2, client_handle2, client_name2);

  // Verify the expired handle was cleaned up
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

// Test for HasClientForClientChannel with expired handle
TEST_F(WebsocketBridgeStateTest, HasClientForClientChannelExpiredHandle) {
  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Should return false for expired handle
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

// Test for HasTopicToClientChannelMapping
TEST_F(WebsocketBridgeStateTest, HasTopicToClientChannelMapping) {
  EXPECT_FALSE(state.hasTopicToClientChannelMapping(topic1));

  const WsClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);

  EXPECT_TRUE(state.hasTopicToClientChannelMapping(topic1));
}

}  // namespace heph::ws::tests
