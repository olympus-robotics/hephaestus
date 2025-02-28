//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <optional>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>

#include "hephaestus/websocket_bridge/bridge_state.h"

namespace heph::ws::tests {

class WsBridgeStateTest : public ::testing::Test {
protected:
  WsBridgeState state;
  WsServerChannelId channel_id1 = 1;
  WsServerChannelId channel_id2 = 2;
  std::string topic1 = "topic1";
  std::string topic2 = "topic2";

  std::shared_ptr<int> client_handle1_shared_ptr;
  std::shared_ptr<int> client_handle2_shared_ptr;

  WsServerClientHandle client_handle1;
  WsServerClientHandle client_handle2;
  std::string client_name1 = "client1";
  std::string client_name2 = "client2";

  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    client_handle1_shared_ptr = std::make_shared<int>(1);
    client_handle2_shared_ptr = std::make_shared<int>(2);
    client_handle1 = client_handle1_shared_ptr;
    client_handle2 = client_handle2_shared_ptr;

    ASSERT_FALSE(client_handle1.expired());
    ASSERT_FALSE(client_handle2.expired());
  }

  void TearDown() override {
  }
};

TEST_F(WsBridgeStateTest, AddAndGetIpcTopicForWsChannel) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_EQ(state.getIpcTopicForWsChannel(channel_id1), topic1);
}

TEST_F(WsBridgeStateTest, GetIpcTopicForWsChannelNotFound) {
  EXPECT_EQ(state.getIpcTopicForWsChannel(channel_id1), "");
}

TEST_F(WsBridgeStateTest, AddAndGetWsChannelForIpcTopic) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_EQ(state.getWsChannelForIpcTopic(topic1), channel_id1);
}

TEST_F(WsBridgeStateTest, GetWsChannelForIpcTopicNotFound) {
  EXPECT_EQ(state.getWsChannelForIpcTopic(topic1), WsServerChannelId{});
}

TEST_F(WsBridgeStateTest, RemoveWsChannelToIpcTopicMapping) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.removeWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_EQ(state.getIpcTopicForWsChannel(channel_id1), "");
  EXPECT_EQ(state.getWsChannelForIpcTopic(topic1), WsServerChannelId{});
}

TEST_F(WsBridgeStateTest, HasWsChannelMapping) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_TRUE(state.hasWsChannelMapping(channel_id1));
  EXPECT_FALSE(state.hasWsChannelMapping(channel_id2));
}

TEST_F(WsBridgeStateTest, HasIpcTopicMapping) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  EXPECT_TRUE(state.hasIpcTopicMapping(topic1));
  EXPECT_FALSE(state.hasIpcTopicMapping(topic2));
}

TEST_F(WsBridgeStateTest, TopicChannelMappingToString) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_NE(state.topicChannelMappingToString().find("[1]"), std::string::npos);
  EXPECT_NE(state.topicChannelMappingToString().find("client1"), std::string::npos);
  EXPECT_NE(state.topicChannelMappingToString().find("topic1"), std::string::npos);
}

TEST_F(WsBridgeStateTest, AddAndGetClientsForWsChannel) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  auto clients = state.getClientsForWsChannel(channel_id1);
  ASSERT_TRUE(clients.has_value());
  EXPECT_EQ(clients->size(), 1);
  EXPECT_EQ(clients->begin()->second, client_name1);
}

TEST_F(WsBridgeStateTest, GetClientsForWsChannelNotFound) {
  auto clients = state.getClientsForWsChannel(channel_id1);
  EXPECT_FALSE(clients.has_value());
}

TEST_F(WsBridgeStateTest, RemoveWsChannelToClientMapping) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  state.removeWsChannelToClientMapping(channel_id1);
  auto clients = state.getClientsForWsChannel(channel_id1);
  EXPECT_FALSE(clients.has_value());
}

TEST_F(WsBridgeStateTest, RemoveSpecificClientFromWsChannel) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  state.addWsChannelToClientMapping(channel_id1, client_handle2, client_name2);
  state.removeWsChannelToClientMapping(channel_id1, client_handle1);
  auto clients = state.getClientsForWsChannel(channel_id1);
  ASSERT_TRUE(clients.has_value());
  EXPECT_EQ(clients->size(), 1);
  EXPECT_EQ(clients->begin()->second, client_name2);
}

TEST_F(WsBridgeStateTest, HasWsChannelWithClients) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_TRUE(state.hasWsChannelWithClients(channel_id1));
  EXPECT_FALSE(state.hasWsChannelWithClients(channel_id2));
}

TEST_F(WsBridgeStateTest, ToString) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_NE(state.toString().find("[1]"), std::string::npos);
  EXPECT_NE(state.toString().find("client1"), std::string::npos);
}

TEST_F(WsBridgeStateTest, AddAndRetrieveServiceMapping) {
  WsServerServiceId serviceId = 101;
  std::string serviceName = "test_service";
  state.addWsServiceToIpcServiceMapping(serviceId, serviceName);

  EXPECT_EQ(state.getIpcServiceForWsService(serviceId), serviceName);
  EXPECT_EQ(state.getWsServiceForIpcService(serviceName), serviceId);
}

TEST_F(WsBridgeStateTest, RemoveServiceMapping) {
  WsServerServiceId serviceId = 202;
  std::string serviceName = "removable_service";
  state.addWsServiceToIpcServiceMapping(serviceId, serviceName);
  state.removeWsServiceToIpcServiceMapping(serviceId, serviceName);

  EXPECT_FALSE(state.hasWsServiceMapping(serviceId));
  EXPECT_FALSE(state.hasIpcServiceMapping(serviceName));
}

TEST_F(WsBridgeStateTest, ServiceMappingToString) {
  WsServerServiceId serviceId = 303;
  std::string serviceName = "string_service";
  state.addWsServiceToIpcServiceMapping(serviceId, serviceName);

  auto mappingStr = state.servicMappingToString();
  EXPECT_NE(mappingStr.find(serviceName), std::string::npos);
}

// Additional tests for WsBridgeState

// Client channel to topic mappings
TEST_F(WsBridgeStateTest, AddAndGetTopicForClientChannel) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_EQ(state.getTopicForClientChannel(client_channel_id), topic1);
}

TEST_F(WsBridgeStateTest, GetClientChannelsForTopic) {
  WsServerClientChannelId client_channel_id1 = 10001;
  WsServerClientChannelId client_channel_id2 = 10002;
  state.addClientChannelToTopicMapping(client_channel_id1, topic1);
  state.addClientChannelToTopicMapping(client_channel_id2, topic1);

  auto client_channels = state.getClientChannelsForTopic(topic1);
  EXPECT_EQ(client_channels.size(), 2);
  EXPECT_TRUE(client_channels.find(client_channel_id1) != client_channels.end());
  EXPECT_TRUE(client_channels.find(client_channel_id2) != client_channels.end());
}

TEST_F(WsBridgeStateTest, GetTopicForClientChannelNotFound) {
  WsServerClientChannelId client_channel_id = 10001;
  EXPECT_EQ(state.getTopicForClientChannel(client_channel_id), "");
}

TEST_F(WsBridgeStateTest, GetClientChannelsForTopicNotFound) {
  auto client_channels = state.getClientChannelsForTopic(topic1);
  EXPECT_TRUE(client_channels.empty());
}

TEST_F(WsBridgeStateTest, RemoveClientChannelToTopicMapping) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_TRUE(state.hasClientChannelMapping(client_channel_id));

  state.removeClientChannelToTopicMapping(client_channel_id);
  EXPECT_FALSE(state.hasClientChannelMapping(client_channel_id));
  EXPECT_FALSE(state.hasTopicToClientChannelMapping(topic1));
}

TEST_F(WsBridgeStateTest, HasClientChannelMapping) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_TRUE(state.hasClientChannelMapping(client_channel_id));
  EXPECT_FALSE(state.hasClientChannelMapping(10002));
}

TEST_F(WsBridgeStateTest, HasWsChannelWithClients_NoClientsInMap) {
  state.addWsChannelToIpcTopicMapping(channel_id2, topic2);
  EXPECT_FALSE(state.hasWsChannelWithClients(channel_id2));
}

TEST_F(WsBridgeStateTest, HasWsChannelWithClients_ExpiredHandle) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  client_handle1_shared_ptr.reset();
  // NOTE: The handle is expired, but without a call to the
  // cleanup function, the entry still persists.
  EXPECT_TRUE(state.hasWsChannelWithClients(channel_id1));
}

// Test for checkConsistency()
TEST_F(WsBridgeStateTest, CheckConsistency_ValidState) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_TRUE(state.checkConsistency());
}

// Tests for WS Service Call ID <-> WS Clients
TEST_F(WsBridgeStateTest, HasCallIdToClientMapping) {
  uint32_t call_id = 5000;
  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(call_id));
}

TEST_F(WsBridgeStateTest, AddAndGetClientForCallId) {
  uint32_t call_id = 5001;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);

  auto client = state.getClientForCallId(call_id);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);
}

TEST_F(WsBridgeStateTest, GetClientForCallIdNotFound) {
  uint32_t call_id = 5002;
  auto client = state.getClientForCallId(call_id);
  EXPECT_FALSE(client.has_value());
}

TEST_F(WsBridgeStateTest, RemoveCallIdToClientMapping) {
  uint32_t call_id = 5003;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(call_id));

  state.removeCallIdToClientMapping(call_id);
  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
}

TEST_F(WsBridgeStateTest, CallIdToClientMappingToString) {
  uint32_t call_id = 5004;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);

  auto mappingStr = state.callIdToClientMappingToString();
  EXPECT_NE(mappingStr.find(std::to_string(call_id)), std::string::npos);
  EXPECT_NE(mappingStr.find(client_name1), std::string::npos);
}

TEST_F(WsBridgeStateTest, CleanUpCallIdToClientMapping_ExpiredHandle) {
  uint32_t call_id = 5005;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(call_id));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Add another mapping to trigger cleanup
  state.addCallIdToClientMapping(5006, client_handle2, client_name2);

  // Verify the expired handle was cleaned up
  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
}

// Test for clientChannelMappingToString
TEST_F(WsBridgeStateTest, ClientChannelMappingToString) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);

  auto mappingStr = state.clientChannelMappingToString();
  EXPECT_NE(mappingStr.find(topic1), std::string::npos);
  EXPECT_NE(mappingStr.find(std::to_string(client_channel_id)), std::string::npos);
  EXPECT_NE(mappingStr.find(client_name1), std::string::npos);
}

// Tests for WS Client Channels <-> WS Clients
TEST_F(WsBridgeStateTest, HasClientForClientChannel) {
  WsServerClientChannelId client_channel_id = 10001;
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));

  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));
}

TEST_F(WsBridgeStateTest, AddAndGetClientForClientChannel) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);

  auto client = state.getClientForClientChannel(client_channel_id);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);
}

TEST_F(WsBridgeStateTest, GetClientForClientChannelNotFound) {
  WsServerClientChannelId client_channel_id = 10001;
  auto client = state.getClientForClientChannel(client_channel_id);
  EXPECT_FALSE(client.has_value());
}

TEST_F(WsBridgeStateTest, RemoveClientChannelToClientMapping) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  state.removeClientChannelToClientMapping(client_channel_id);
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

TEST_F(WsBridgeStateTest, CleanUpClientChannelToClientMapping_ExpiredHandle) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Add another mapping to trigger cleanup
  state.addClientChannelToClientMapping(10002, client_handle2, client_name2);

  // Verify the expired handle was cleaned up
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

// Test for HasClientForClientChannel with expired handle
TEST_F(WsBridgeStateTest, HasClientForClientChannel_ExpiredHandle) {
  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Should return false for expired handle
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

// Test for HasTopicToClientChannelMapping
TEST_F(WsBridgeStateTest, HasTopicToClientChannelMapping) {
  EXPECT_FALSE(state.hasTopicToClientChannelMapping(topic1));

  WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);

  EXPECT_TRUE(state.hasTopicToClientChannelMapping(topic1));
}

}  // namespace heph::ws::tests