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
;

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

}  // namespace heph::ws_bridge::tests