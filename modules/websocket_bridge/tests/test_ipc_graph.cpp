#include <optional>
#include <string>

#include <gtest/gtest.h>
#include <hephaestus/ipc/ipc_graph.h>
#include <hephaestus/ipc/topic.h>
#include <hephaestus/ipc/zenoh/publisher.h>
#include <hephaestus/ipc/zenoh/raw_subscriber.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/types/dummy_type.h>
#include <hephaestus/types_proto/dummy_type.h>  // NOLINT(misc-include-cleaner)

namespace heph::ws_bridge::tests {

class IpcGraphTest : public ::testing::Test {
protected:
  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
  }

  void TearDown() override {
    graph->stop();
  }

  IpcGraphConfig config;
  std::unique_ptr<IpcGraph> graph;

  std::shared_ptr<ipc::zenoh::Session> pub_session;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::Publisher<types::DummyType>>> pub_map;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawSubscriber>> sub_map;

  void startIpcGraph() {
    config.session = heph::ipc::zenoh::createSession(heph::ipc::zenoh::createLocalConfig());
    graph = std::make_unique<IpcGraph>(config);
    graph->start();
  }

  void createTestPublisher(const std::string& topic) {
    const auto pub_topic = ipc::TopicConfig(topic);
    pub_map[pub_topic.name] =
        std::make_unique<ipc::zenoh::Publisher<types::DummyType>>(config.session, pub_topic);
  }

  void createTestSubscriber(const std::string& topic) {
    const auto sub_topic = ipc::TopicConfig(topic);
    sub_map[sub_topic.name] = std::make_unique<ipc::zenoh::RawSubscriber>(
        config.session, sub_topic, [](const ipc::zenoh::MessageMetadata&, std::span<const std::byte>) {});
  }
};

TEST_F(IpcGraphTest, StartStop) {
  startIpcGraph();

  // Do nothing, especially not crash.
}

TEST_F(IpcGraphTest, PublisherEventTriggersCallbacksAndGraphUpdate) {
  bool publisher_added = false;
  bool publisher_removed = false;
  bool graph_updated = false;
  IpcGraphState last_state;

  config.graph_update_cb = [&](const IpcGraphState& state) {
    graph_updated = true;
    last_state = state;
  };

  config.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == "test_topic") {
      publisher_added = true;
    }
  };

  config.topic_removal_cb = [&](const std::string& topic) {
    if (topic == "test_topic") {
      publisher_removed = true;
    }
  };

  startIpcGraph();

  EXPECT_TRUE(last_state.topics_to_types_map.empty());
  EXPECT_TRUE(last_state.topic_to_publishers_map.empty());

  EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());
  EXPECT_TRUE(last_state.services_to_types_map.empty());
  EXPECT_TRUE(last_state.services_to_nodes_map.empty());

  createTestPublisher("test_topic");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(publisher_added);
  EXPECT_TRUE(graph_updated);
  graph_updated = false;

  EXPECT_FALSE(last_state.topics_to_types_map.empty());
  EXPECT_EQ(last_state.topics_to_types_map["test_topic"], "heph.types.proto.DummyType");
  EXPECT_FALSE(last_state.topic_to_publishers_map.empty());
  EXPECT_EQ(last_state.topic_to_publishers_map["test_topic"].size(), 1);

  EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());
  EXPECT_TRUE(last_state.services_to_types_map.empty());
  EXPECT_TRUE(last_state.services_to_nodes_map.empty());

  pub_map["test_topic"].reset();  // Simulate publisher removal

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(publisher_removed);
  EXPECT_TRUE(graph_updated);

  EXPECT_TRUE(last_state.topics_to_types_map.empty());
  EXPECT_TRUE(last_state.topic_to_publishers_map.empty());

  EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());
  EXPECT_TRUE(last_state.services_to_types_map.empty());
  EXPECT_TRUE(last_state.services_to_nodes_map.empty());
}

TEST_F(IpcGraphTest, SubscriberEventTriggersGraphUpdate) {
  bool graph_updated = false;
  IpcGraphState last_state;

  config.graph_update_cb = [&](const IpcGraphState& state) {
    graph_updated = true;
    last_state = state;
  };

  startIpcGraph();

  EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());

  EXPECT_TRUE(last_state.topics_to_types_map.empty());
  EXPECT_TRUE(last_state.topic_to_publishers_map.empty());
  EXPECT_TRUE(last_state.services_to_types_map.empty());
  EXPECT_TRUE(last_state.services_to_nodes_map.empty());

  createTestSubscriber("test_topic");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(graph_updated);
  graph_updated = false;

  EXPECT_FALSE(last_state.topic_to_subscribers_map.empty());
  EXPECT_EQ(last_state.topic_to_subscribers_map["test_topic"].size(), 1);

  EXPECT_TRUE(last_state.topics_to_types_map.empty());
  EXPECT_TRUE(last_state.topic_to_publishers_map.empty());
  EXPECT_TRUE(last_state.services_to_types_map.empty());
  EXPECT_TRUE(last_state.services_to_nodes_map.empty());

  sub_map["test_topic"].reset();  // Simulate subscriber removal

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(graph_updated);

  EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());

  EXPECT_TRUE(last_state.topics_to_types_map.empty());
  EXPECT_TRUE(last_state.topic_to_publishers_map.empty());
  EXPECT_TRUE(last_state.services_to_types_map.empty());
  EXPECT_TRUE(last_state.services_to_nodes_map.empty());
}

TEST_F(IpcGraphTest, GetTopicTypeInfo) {
  bool topic_discovered = false;

  config.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == "test_topic") {
      topic_discovered = true;
    }
  };

  startIpcGraph();

  createTestPublisher("test_topic");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(topic_discovered);

  auto type_info = graph->getTopicTypeInfo("test_topic");
  EXPECT_TRUE(type_info.has_value());
  EXPECT_EQ(type_info->name, "heph.types.proto.DummyType");
}

TEST_F(IpcGraphTest, GetTopicListString) {
  bool topic_discovered = false;

  config.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == "test_topic") {
      topic_discovered = true;
    }
  };

  startIpcGraph();

  createTestPublisher("test_topic");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(topic_discovered);

  auto topic_list = graph->getTopicListString();
  EXPECT_FALSE(topic_list.empty());
}

TEST_F(IpcGraphTest, GetMaps) {
  bool topic_discovered = false;

  config.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == "test_topic") {
      topic_discovered = true;
    }
  };

  startIpcGraph();

  createTestPublisher("test_topic");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(topic_discovered);

  auto topics_to_types = graph->getTopicsToTypesMap();
  EXPECT_FALSE(topics_to_types.empty());

  auto topic_to_publishers = graph->getTopicToPublishersMap();
  EXPECT_FALSE(topic_to_publishers.empty());

  auto topic_to_subscribers = graph->getTopicToSubscribersMap();
  EXPECT_TRUE(topic_to_subscribers.empty());

  auto services_to_types = graph->getServicesToTypesMap();
  EXPECT_TRUE(services_to_types.empty());

  auto services_to_nodes = graph->getServicesToNodesMap();
  EXPECT_TRUE(services_to_nodes.empty());
}

}  // namespace heph::ws_bridge::tests
