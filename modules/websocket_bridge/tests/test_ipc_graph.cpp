#include <optional>
#include <string>

#include <gtest/gtest.h>
#include <hephaestus/ipc/ipc_graph.h>
#include <hephaestus/ipc/topic.h>
#include <hephaestus/ipc/zenoh/publisher.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/ipc/zenoh/subscriber.h>
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

  std::unordered_map<std::string, std::vector<std::unique_ptr<ipc::zenoh::Publisher<types::DummyType>>>>
      pub_map;
  std::unordered_map<std::string, std::vector<std::unique_ptr<ipc::zenoh::Subscriber<types::DummyType>>>>
      sub_map;

  void startIpcGraph() {
    // Note: We deliberately are not using createLocalConfig, because we want those
    // sessions to talk to each other. Within a single session, only the first publisher/subscriber/service to
    // start and the last to die of the same topic/type will create a observable liveliness event. Across
    // multiple sessions, every new liveliness event is tracked.
    // So we want multicast scouting enabled.
    config.session = heph::ipc::zenoh::createSession(heph::ipc::zenoh::Config());
    graph = std::make_unique<IpcGraph>(config);
    graph->start();
  }

  void createTestPublisher(const std::string& topic) {
    auto pub_session = heph::ipc::zenoh::createSession(heph::ipc::zenoh::Config());

    const auto pub_topic = ipc::TopicConfig(topic);
    pub_map[pub_topic.name].emplace_back(
        std::make_unique<ipc::zenoh::Publisher<types::DummyType>>(std::move(pub_session), pub_topic));
  }

  void createTestSubscriber(const std::string& topic) {
    auto sub_session = heph::ipc::zenoh::createSession(heph::ipc::zenoh::Config());

    const auto sub_topic = ipc::TopicConfig(topic);

    sub_map[sub_topic.name].emplace_back(heph::ipc::zenoh::createSubscriber<types::DummyType>(
        sub_session, sub_topic,
        [](const heph::ipc::zenoh::MessageMetadata& metadata, const std::shared_ptr<types::DummyType>& pose) {
          (void)metadata;
          (void)pose;
        }));
  }

  void sleepLongEnoughToSync() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
};

TEST_F(IpcGraphTest, StartStop) {
  startIpcGraph();

  // Do nothing, especially not crash.
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

  sleepLongEnoughToSync();

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

  sleepLongEnoughToSync();

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

  sleepLongEnoughToSync();

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

TEST_F(IpcGraphTest, PubSubCreationAndDeletionEvents) {
  const int num_iterations = 2;
  const int num_topics = 3;
  const std::string topic_prefix = "unique_test_topic_";

  int event_counter_topic_discovery = 0;
  int event_counter_topic_removal = 0;
  int event_counter_graph_update = 0;

  IpcGraphState last_state;

  config.graph_update_cb = [&](const IpcGraphState& state) {
    ++event_counter_graph_update;
    last_state = state;
  };

  config.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic.rfind(topic_prefix, 0) == 0) {
      ++event_counter_topic_discovery;
    }
  };

  config.topic_removal_cb = [&](const std::string& topic) {
    if (topic.rfind(topic_prefix, 0) == 0) {
      ++event_counter_topic_removal;
    }
  };

  startIpcGraph();

  sleepLongEnoughToSync();

  for (int i = 0; i < num_iterations; ++i) {
    const int num_discovery_events = event_counter_topic_discovery;
    const int num_removal_events = event_counter_topic_removal;
    const int num_graph_update_events = event_counter_graph_update;

    // Create pubs & subs
    for (int j = 0; j < num_topics; ++j) {
      auto topic = topic_prefix + std::to_string(i) + "_" + std::to_string(j);

      createTestPublisher(topic);
      createTestPublisher(topic);
      createTestSubscriber(topic);
      createTestSubscriber(topic);
    }

    sleepLongEnoughToSync();

    // Verify state after pub/sub Creation
    {
      // Should have caught a discovery and TWO graph update events for each topic
      EXPECT_EQ(event_counter_graph_update, num_graph_update_events + (4 * num_topics));
      EXPECT_EQ(event_counter_topic_discovery, num_discovery_events + num_topics);
      EXPECT_EQ(event_counter_topic_removal, num_removal_events);

      // There should now be pubs, sub and type mappings for each topic
      EXPECT_EQ(last_state.topic_to_publishers_map.size(), num_topics);
      EXPECT_EQ(last_state.topic_to_subscribers_map.size(), num_topics);
      EXPECT_EQ(last_state.topics_to_types_map.size(), num_topics);
    }

    // Remove subs

    {
      sub_map.clear();
    }

    sleepLongEnoughToSync();

    // Verify state after sub deletion
    {
      // Should have caught an additional graph update event for each topic and sub removed, no other events
      EXPECT_EQ(event_counter_graph_update, num_graph_update_events + (6 * num_topics));
      EXPECT_EQ(event_counter_topic_discovery, num_discovery_events + num_topics);
      EXPECT_EQ(event_counter_topic_removal, num_removal_events);

      // There should now no longer be subs, but everything else stays the same
      EXPECT_EQ(last_state.topic_to_publishers_map.size(), num_topics);
      EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());
      EXPECT_EQ(last_state.topics_to_types_map.size(), num_topics);
    }

    // Remove the pubs
    {
      pub_map.clear();
    }

    sleepLongEnoughToSync();

    // Verify state after pub deletion
    {
      // Should have caught a removal and an additional graph update event for each topic
      EXPECT_EQ(event_counter_graph_update, num_graph_update_events + (8 * num_topics));
      EXPECT_EQ(event_counter_topic_discovery, num_discovery_events + num_topics);
      EXPECT_EQ(event_counter_topic_removal, num_removal_events + num_topics);

      // There should now no longer be any pub, sub or type mappings
      EXPECT_TRUE(last_state.topic_to_publishers_map.empty());
      EXPECT_TRUE(last_state.topic_to_subscribers_map.empty());
      EXPECT_TRUE(last_state.topics_to_types_map.empty());
    }
  }
}

}  // namespace heph::ws_bridge::tests
