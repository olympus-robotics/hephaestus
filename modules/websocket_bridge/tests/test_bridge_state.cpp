#include <optional>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

#include "hephaestus/websocket_bridge/bridge_state.h"

namespace heph::ws_bridge::tests {

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
  std::string expected = "  IPC Topic to WS Channel Mapping:\n  \t'topic1' -> [1]\n";
  EXPECT_EQ(state.topicChannelMappingToString(), expected);
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

TEST_F(WsBridgeStateTest, ChannelClientMappingToString) {
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  EXPECT_NE(state.channelClientMappingToString().find("[1]"), std::string::npos);
  EXPECT_NE(state.channelClientMappingToString().find("client1"), std::string::npos);
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

TEST_F(WsBridgeStateTest, CheckConsistency) {
  EXPECT_TRUE(state.checkConsistency());
}

TEST_F(WsBridgeStateTest, AddAndRetrieveCallIdToClientMapping) {
  uint32_t call_id = 1;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);

  auto client = state.getClientForCallId(call_id);
  ASSERT_TRUE(client.has_value());
  EXPECT_EQ(client->second, client_name1);
}

TEST_F(WsBridgeStateTest, RemoveCallIdToClientMapping) {
  uint32_t call_id = 2;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);
  state.removeCallIdToClientMapping(call_id);

  EXPECT_FALSE(state.hasCallIdToClientMapping(call_id));
}

TEST_F(WsBridgeStateTest, CallIdToClientMappingToString) {
  uint32_t call_id = 3;
  state.addCallIdToClientMapping(call_id, client_handle1, client_name1);

  auto mappingStr = state.callIdToClientMappingToString();
  EXPECT_NE(mappingStr.find("client1"), std::string::npos);
}

TEST_F(WsBridgeStateTest, PrintBridgeState) {
  state.addWsChannelToIpcTopicMapping(channel_id1, topic1);
  state.addWsChannelToClientMapping(channel_id1, client_handle1, client_name1);
  state.addWsServiceToIpcServiceMapping(101, "test_service");
  state.addCallIdToClientMapping(1, client_handle1, client_name1);

  testing::internal::CaptureStdout();
  state.printBridgeState();
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("[1]"), std::string::npos);
  EXPECT_NE(output.find("client1"), std::string::npos);
  EXPECT_NE(output.find("test_service"), std::string::npos);
}

}  // namespace heph::ws_bridge::tests