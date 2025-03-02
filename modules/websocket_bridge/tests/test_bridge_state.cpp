//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <optional>
#include <string>

#include <gtest/gtest.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>

#include "hephaestus/websocket_bridge/bridge_state.h"

namespace heph::ws::tests {

class WsBridgeStateTest : public ::testing::Test {
protected:
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

  // NOLINTBEGIN
  // Note: The following variables are perfectly ok to be accessible from within the test.
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
  // NOLINTEND
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
  const WsServerServiceId service_id = 101;
  const std::string service_name = "test_service";
  state.addWsServiceToIpcServiceMapping(service_id, service_name);

  EXPECT_EQ(state.getIpcServiceForWsService(service_id), service_name);
  EXPECT_EQ(state.getWsServiceForIpcService(service_name), service_id);
}

TEST_F(WsBridgeStateTest, RemoveServiceMapping) {
  const WsServerServiceId service_id = 202;
  const std::string service_name = "removable_service";
  state.addWsServiceToIpcServiceMapping(service_id, service_name);
  state.removeWsServiceToIpcServiceMapping(service_id, service_name);

  EXPECT_FALSE(state.hasWsServiceMapping(service_id));
  EXPECT_FALSE(state.hasIpcServiceMapping(service_name));
}

TEST_F(WsBridgeStateTest, ServiceMappingToString) {
  const WsServerServiceId service_id = 303;
  const std::string service_name = "string_service";
  state.addWsServiceToIpcServiceMapping(service_id, service_name);

  auto mapping_str = state.servicMappingToString();
  EXPECT_NE(mapping_str.find(service_name), std::string::npos);
}

// Additional tests for WsBridgeState

// Client channel to topic mappings
TEST_F(WsBridgeStateTest, AddAndGetTopicForClientChannel) {
  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_EQ(state.getTopicForClientChannel(client_channel_id), topic1);
}

TEST_F(WsBridgeStateTest, GetClientChannelsForTopic) {
  const WsServerClientChannelId client_channel_id1 = 10001;
  const WsServerClientChannelId client_channel_id2 = 10002;
  state.addClientChannelToTopicMapping(client_channel_id1, topic1);
  state.addClientChannelToTopicMapping(client_channel_id2, topic1);

  auto client_channels = state.getClientChannelsForTopic(topic1);
  EXPECT_EQ(client_channels.size(), 2);
  EXPECT_TRUE(client_channels.find(client_channel_id1) != client_channels.end());
  EXPECT_TRUE(client_channels.find(client_channel_id2) != client_channels.end());
}

TEST_F(WsBridgeStateTest, GetTopicForClientChannelNotFound) {
  const WsServerClientChannelId client_channel_id = 10001;
  EXPECT_EQ(state.getTopicForClientChannel(client_channel_id), "");
}

TEST_F(WsBridgeStateTest, GetClientChannelsForTopicNotFound) {
  auto client_channels = state.getClientChannelsForTopic(topic1);
  EXPECT_TRUE(client_channels.empty());
}

TEST_F(WsBridgeStateTest, RemoveClientChannelToTopicMapping) {
  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_TRUE(state.hasClientChannelMapping(client_channel_id));

  state.removeClientChannelToTopicMapping(client_channel_id);
  EXPECT_FALSE(state.hasClientChannelMapping(client_channel_id));
  EXPECT_FALSE(state.hasTopicToClientChannelMapping(topic1));
}

TEST_F(WsBridgeStateTest, HasClientChannelMapping) {
  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  EXPECT_TRUE(state.hasClientChannelMapping(client_channel_id));
  EXPECT_FALSE(state.hasClientChannelMapping(10002));
}

TEST_F(WsBridgeStateTest, HasWsChannelWithClientsNoClientsInMap) {
  state.addWsChannelToIpcTopicMapping(channel_id2, topic2);
  EXPECT_FALSE(state.hasWsChannelWithClients(channel_id2));
}

TEST_F(WsBridgeStateTest, HasWsChannelWithClientsExpiredHandle) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  client_handle1_shared_ptr.reset();
  // NOTE: The handle is expired, but without a call to the
  // cleanup function, the entry still persists.
  EXPECT_TRUE(state.hasWsChannelWithClients(channel_id1));
}

// Test for checkConsistency()
TEST_F(WsBridgeStateTest, CheckConsistencyValidState) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_TRUE(state.checkConsistency());
}

// Tests for WS Service Call ID <-> WS Clients
TEST_F(WsBridgeStateTest, HasCallIdToClientMapping) {
  const uint32_t call_id = 5000;
  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(call_id));
}

TEST_F(WsBridgeStateTest, AddAndGetClientForCallId) {
  const uint32_t call_id = 5001;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);

  auto client = state.getClientForCallId(call_id);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);
}

TEST_F(WsBridgeStateTest, GetClientForCallIdNotFound) {
  const uint32_t call_id = 5002;
  auto client = state.getClientForCallId(call_id);
  EXPECT_FALSE(client.has_value());
}

TEST_F(WsBridgeStateTest, RemoveCallIdToClientMapping) {
  const uint32_t call_id = 5003;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(call_id));

  state.removeCallIdToClientMapping(call_id);
  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
}

TEST_F(WsBridgeStateTest, CallIdToClientMappingToString) {
  const uint32_t call_id = 5004;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);

  auto mapping_str = state.callIdToClientMappingToString();
  EXPECT_NE(mapping_str.find(std::to_string(call_id)), std::string::npos);
  EXPECT_NE(mapping_str.find(client_name1), std::string::npos);
}

TEST_F(WsBridgeStateTest, CleanUpCallIdToClientMappingExpiredHandle) {
  const uint32_t call_id = 5005;
  const uint32_t call_id_2 = 5006;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasCallIdToClientMapping(call_id));

  // Expire the client handle
  client_handle1_shared_ptr.reset();

  // Add another mapping to trigger cleanup
  state.addCallIdToClientMapping(call_id_2, client_handle2, client_name2);

  // Verify the expired handle was cleaned up
  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
}

// Test for clientChannelMappingToString
TEST_F(WsBridgeStateTest, ClientChannelMappingToString) {
  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);

  auto mapping_str = state.clientChannelMappingToString();
  EXPECT_NE(mapping_str.find(topic1), std::string::npos);
  EXPECT_NE(mapping_str.find(std::to_string(client_channel_id)), std::string::npos);
  EXPECT_NE(mapping_str.find(client_name1), std::string::npos);
}

// Tests for WS Client Channels <-> WS Clients
TEST_F(WsBridgeStateTest, HasClientForClientChannel) {
  const WsServerClientChannelId client_channel_id = 10001;
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));

  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));
}

TEST_F(WsBridgeStateTest, AddAndGetClientForClientChannel) {
  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);

  auto client = state.getClientForClientChannel(client_channel_id);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);
}

TEST_F(WsBridgeStateTest, GetClientForClientChannelNotFound) {
  const WsServerClientChannelId client_channel_id = 10001;
  auto client = state.getClientForClientChannel(client_channel_id);
  EXPECT_FALSE(client.has_value());
}

TEST_F(WsBridgeStateTest, RemoveClientChannelToClientMapping) {
  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToClientMapping(client_channel_id, client_handle1, client_name1);
  EXPECT_TRUE(state.hasClientForClientChannel(client_channel_id));

  state.removeClientChannelToClientMapping(client_channel_id);
  EXPECT_FALSE(state.hasClientForClientChannel(client_channel_id));
}

TEST_F(WsBridgeStateTest, CleanUpClientChannelToClientMappingExpiredHandle) {
  const WsServerClientChannelId client_channel_id = 10001;
  const WsServerClientChannelId client_channel_id_2 = 10002;

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
TEST_F(WsBridgeStateTest, HasClientForClientChannelExpiredHandle) {
  const WsServerClientChannelId client_channel_id = 10001;
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

  const WsServerClientChannelId client_channel_id = 10001;
  state.addClientChannelToTopicMapping(client_channel_id, topic1);

  EXPECT_TRUE(state.hasTopicToClientChannelMapping(topic1));
}

}  // namespace heph::ws::tests