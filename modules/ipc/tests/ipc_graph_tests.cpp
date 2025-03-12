//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <optional>
#include <string>

#include <gtest/gtest.h>
#include <hephaestus/ipc/topic.h>
#include <hephaestus/ipc/zenoh/ipc_graph.h>
#include <hephaestus/ipc/zenoh/publisher.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <hephaestus/ipc/zenoh/service_client.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/ipc/zenoh/subscriber.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/types/dummy_type.h>
#include <hephaestus/types_proto/dummy_type.h>  // NOLINT(misc-include-cleaner)

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::tests {
namespace {

class IpcGraphTest : public ::testing::Test {
protected:
  // NOLINTBEGIN
  IpcGraphConfig config;
  IpcGraphCallbacks callbacks;
  std::unique_ptr<IpcGraph> graph;

  using PublisherPtr = std::unique_ptr<ipc::zenoh::Publisher<types::DummyType>>;
  using SubscriberPtr = std::unique_ptr<ipc::zenoh::Subscriber<types::DummyType>>;
  using ServiceServerPtr = std::unique_ptr<ipc::zenoh::Service<types::DummyType, types::DummyType>>;
  using ServiceClientPtr = std::unique_ptr<ipc::zenoh::ServiceClient<types::DummyType, types::DummyType>>;

  std::unordered_map<std::string, std::vector<PublisherPtr>> pub_map;
  std::unordered_map<std::string, std::vector<SubscriberPtr>> sub_map;
  std::unordered_map<std::string, std::vector<ServiceServerPtr>> server_map;
  std::unordered_map<std::string, std::vector<ServiceClientPtr>> client_map;

  const std::string TEST_TOPIC = "test_topic";

  const std::string TEST_PUBLISHER_1 = "test_pub_1";
  const std::string TEST_PUBLISHER_2 = "test_pub_2";
  const std::string TEST_SUBSCRIBER_1 = "test_sub_1";
  const std::string TEST_SUBSCRIBER_2 = "test_sub_2";

  const std::string TEST_SERVICE = "test_service";

  const std::string TEST_SERVICE_SERVER_1 = "test_srv_s_1";
  const std::string TEST_SERVICE_SERVER_2 = "test_srv_s_2";
  const std::string TEST_SERVICE_CLIENT_1 = "test_srv_c_1";
  const std::string TEST_SERVICE_CLIENT_2 = "test_srv_c_2";
  // NOLINTEND

  void startIpcGraph() {
    // Note: We deliberately are not using createLocalConfig, because we want those
    // sessions to talk to each other. Per session only the first to create and last to delete a liveliness
    // token will trigger an update. This is a limitation of hephaestus, not the IPC Graph class due to the
    // way the liveliness tokens are created. So we need one session per publisher and subscriber. So we want
    // multicast scouting enabled to simulate a real world scenario where endpoints are very likely in
    // different sessions.
    config.session = heph::ipc::zenoh::createSession(heph::ipc::zenoh::Config());
    graph = std::make_unique<IpcGraph>(config, std::move(callbacks));
    graph->start();
  }

  void createTestPublisher(const std::string& topic, const std::string& session_name = "test_publisher") {
    auto zenoh_config = heph::ipc::zenoh::Config();
    zenoh_config.id = session_name;
    auto session = heph::ipc::zenoh::createSession(zenoh_config);

    const auto pub_topic = ipc::TopicConfig(topic);
    pub_map[session_name + "|" + topic].emplace_back(
        std::make_unique<ipc::zenoh::Publisher<types::DummyType>>(std::move(session), pub_topic));
  }

  void createTestSubscriber(const std::string& topic, const std::string& session_name = "test_subscriber") {
    auto zenoh_config = heph::ipc::zenoh::Config();
    zenoh_config.id = session_name;
    auto session = heph::ipc::zenoh::createSession(zenoh_config);

    const auto sub_topic = ipc::TopicConfig(topic);

    sub_map[session_name + "|" + topic].emplace_back(heph::ipc::zenoh::createSubscriber<types::DummyType>(
        session, sub_topic,
        [](const heph::ipc::zenoh::MessageMetadata& metadata, const std::shared_ptr<types::DummyType>& pose) {
          (void)metadata;
          (void)pose;
        }));
  }

  void createTestServiceServer(const std::string& service,
                               const std::string& session_name = "test_service_server") {
    auto zenoh_config = heph::ipc::zenoh::Config();
    zenoh_config.id = session_name;
    auto session = heph::ipc::zenoh::createSession(zenoh_config);

    const ipc::TopicConfig service_topic_config(service);
    server_map[session_name + "|" + service].emplace_back(
        std::make_unique<ipc::zenoh::Service<types::DummyType, types::DummyType>>(
            std::move(session), service_topic_config,
            [](const types::DummyType& request) -> types::DummyType {
              // Process the request and return a response
              return request;
            }));
  }

  void createTestServiceClient(const std::string& service,
                               const std::string& session_name = "test_service_client") {
    const constexpr int TIMEOUT_MS = 20;

    auto zenoh_config = heph::ipc::zenoh::Config();
    zenoh_config.id = session_name;
    auto session = heph::ipc::zenoh::createSession(zenoh_config);

    const auto service_topic_config = ipc::TopicConfig(service);
    client_map[session_name + "|" + service].emplace_back(
        std::make_unique<ipc::zenoh::ServiceClient<types::DummyType, types::DummyType>>(
            std::move(session), service_topic_config, std::chrono::milliseconds(TIMEOUT_MS)));
  }

  static void sleepLongEnoughToSync() {
    const constexpr int SLEEP_DURATION_MS = 20;
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_MS));
  }

  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
  }

  void TearDown() override {
    graph->stop();
    graph.reset();
    pub_map.clear();
    sub_map.clear();
    server_map.clear();
    client_map.clear();
  }
};

TEST_F(IpcGraphTest, TopicDiscoveryAndRemovalWithoutSubTopicTracking) {
  bool topic_discovered = false;
  bool topic_removed = false;
  bool graph_updated = false;

  callbacks.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == TEST_TOPIC) {
      topic_discovered = true;
    }
  };

  callbacks.topic_removal_cb = [&](const std::string& topic) {
    if (topic == TEST_TOPIC) {
      topic_removed = true;
    }
  };

  callbacks.graph_update_cb = [&](const auto& info, const IpcGraphState& state) {
    if (info.topic == TEST_TOPIC) {
      EXPECT_TRUE(state.checkConsistency());
      state.printIpcGraphState();
      graph_updated = true;
    }
  };

  // THIS IS NOT THE DEFAULT BEHAVIOR
  config.track_topics_based_on_subscribers = false;

  auto reset = [&]() {
    topic_discovered = false;
    topic_removed = false;
    graph_updated = false;
  };

  startIpcGraph();

  IpcGraphTest::sleepLongEnoughToSync();

  reset();

  ASSERT_FALSE(topic_removed);
  ASSERT_FALSE(topic_discovered);
  ASSERT_FALSE(graph_updated);
  ASSERT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  ASSERT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  ASSERT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);

  ////////////////
  // PUBLISHERS //
  ////////////////

  // ADDING FIRST PUBLISHER
  createTestPublisher(TEST_TOPIC, TEST_PUBLISHER_1);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding the first publisher triggers the discovery event.
  EXPECT_EQ(graph->getTopicToPublishersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_TRUE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // ADDING SECOND PUBLISHER
  createTestPublisher(TEST_TOPIC, TEST_PUBLISHER_2);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding a second publisher does not trigger an event.
  EXPECT_EQ(graph->getTopicToPublishersMap()[TEST_TOPIC].size(), 2);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING SECOND PUBLISHER
  pub_map.erase(TEST_PUBLISHER_2 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing a publisher that is not the last will not trigger a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING FIRST PUBLISHER
  pub_map.erase(TEST_PUBLISHER_1 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing the last publisher triggers a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_TRUE(topic_removed);
  EXPECT_TRUE(graph_updated);

  reset();

  ASSERT_FALSE(topic_removed);
  ASSERT_FALSE(topic_discovered);
  ASSERT_FALSE(graph_updated);
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);

  /////////////////
  // SUBSCRIBERS //
  /////////////////

  // ADD FIRST SUBSCRIBER
  createTestSubscriber(TEST_TOPIC, TEST_SUBSCRIBER_1);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding the first subscriber does not trigger a discovery event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // ADDING SECOND SUBSCRIBER
  createTestSubscriber(TEST_TOPIC, TEST_SUBSCRIBER_2);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding a second subscriber does not trigger an event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap()[TEST_TOPIC].size(), 2);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING SECOND SUBSCRIBER
  sub_map.erase(TEST_SUBSCRIBER_2 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing a subscriber that is not the last will not trigger a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING FIRST SUBSCRIBER
  sub_map.erase(TEST_SUBSCRIBER_1 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing the last subscriber does not trigger a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();
}

TEST_F(IpcGraphTest, TopicDiscoveryAndRemovalWithSubTopicTracking) {
  bool topic_discovered = false;
  bool topic_removed = false;
  bool graph_updated = false;

  callbacks.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == TEST_TOPIC) {
      topic_discovered = true;
    }
  };

  callbacks.topic_removal_cb = [&](const std::string& topic) {
    if (topic == TEST_TOPIC) {
      topic_removed = true;
    }
  };

  callbacks.graph_update_cb = [&](const auto& info, const IpcGraphState& state) {
    if (info.topic == TEST_TOPIC) {
      EXPECT_TRUE(state.checkConsistency());
      state.printIpcGraphState();
      graph_updated = true;
    }
  };

  // THIS IS NOT THE DEFAULT BEHAVIOR
  config.track_topics_based_on_subscribers = true;

  auto reset = [&]() {
    topic_discovered = false;
    topic_removed = false;
    graph_updated = false;
  };

  startIpcGraph();

  IpcGraphTest::sleepLongEnoughToSync();

  reset();

  ASSERT_FALSE(topic_removed);
  ASSERT_FALSE(topic_discovered);
  ASSERT_FALSE(graph_updated);
  ASSERT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  ASSERT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  ASSERT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);

  ////////////////
  // PUBLISHERS //
  ////////////////

  // ADDING FIRST PUBLISHER
  createTestPublisher(TEST_TOPIC, TEST_PUBLISHER_1);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding the first publisher triggers the discovery event.
  EXPECT_EQ(graph->getTopicToPublishersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_TRUE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // ADDING SECOND PUBLISHER
  createTestPublisher(TEST_TOPIC, TEST_PUBLISHER_2);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding a second publisher does not trigger an event.
  EXPECT_EQ(graph->getTopicToPublishersMap()[TEST_TOPIC].size(), 2);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING SECOND PUBLISHER
  pub_map.erase(TEST_PUBLISHER_2 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing a publisher that is not the last will not trigger a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING FIRST PUBLISHER
  pub_map.erase(TEST_PUBLISHER_1 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing the last publisher triggers a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_TRUE(topic_removed);
  EXPECT_TRUE(graph_updated);

  reset();

  ASSERT_FALSE(topic_removed);
  ASSERT_FALSE(topic_discovered);
  ASSERT_FALSE(graph_updated);
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);

  /////////////////
  // SUBSCRIBERS //
  /////////////////

  // ADD FIRST SUBSCRIBER
  createTestSubscriber(TEST_TOPIC, TEST_SUBSCRIBER_1);
  IpcGraphTest::sleepLongEnoughToSync();

  // This will now trigger a discovery event!
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_TRUE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // ADDING SECOND SUBSCRIBER
  createTestSubscriber(TEST_TOPIC, TEST_SUBSCRIBER_2);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding a second subscriber does not trigger an event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap()[TEST_TOPIC].size(), 2);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING SECOND SUBSCRIBER
  sub_map.erase(TEST_SUBSCRIBER_2 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing a subscriber that is not the last will not trigger a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap()[TEST_TOPIC].size(), 1);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 1);
  EXPECT_FALSE(topic_discovered);
  EXPECT_FALSE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING FIRST SUBSCRIBER
  sub_map.erase(TEST_SUBSCRIBER_1 + "|" + TEST_TOPIC);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing the last subscriber does not trigger a removal event.
  EXPECT_EQ(graph->getTopicToPublishersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicToSubscribersMap().count(TEST_TOPIC), 0);
  EXPECT_EQ(graph->getTopicsToTypeMap().count(TEST_TOPIC), 0);
  EXPECT_FALSE(topic_discovered);
  EXPECT_TRUE(topic_removed);
  EXPECT_TRUE(graph_updated);
  reset();
}

TEST_F(IpcGraphTest, ServiceDiscoveryAndRemoval) {
  bool service_discovered = false;
  bool service_removed = false;
  bool graph_updated = false;

  callbacks.service_discovery_cb = [&](const std::string& service, const heph::serdes::ServiceTypeInfo&) {
    if (service == TEST_SERVICE) {
      service_discovered = true;
    }
  };

  callbacks.service_removal_cb = [&](const std::string& service) {
    if (service == TEST_SERVICE) {
      service_removed = true;
    }
  };

  callbacks.graph_update_cb = [&](const ipc::zenoh::EndpointInfo& info, const IpcGraphState& state) {
    if (info.topic == TEST_SERVICE) {
      EXPECT_TRUE(state.checkConsistency());
      state.printIpcGraphState();
      graph_updated = true;
    }
  };

  auto reset = [&]() {
    service_discovered = false;
    service_removed = false;
    graph_updated = false;
  };

  startIpcGraph();

  IpcGraphTest::sleepLongEnoughToSync();

  reset();

  ASSERT_FALSE(service_removed);
  ASSERT_FALSE(service_discovered);
  ASSERT_FALSE(graph_updated);
  ASSERT_EQ(graph->getServicesToServersMap().count(TEST_SERVICE), 0);
  ASSERT_EQ(graph->getServicesToClientsMap().count(TEST_SERVICE), 0);
  ASSERT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 0);

  /////////////
  // SERVERS //
  /////////////

  // ADD FIRST SERVER
  createTestServiceServer(TEST_SERVICE, TEST_SERVICE_SERVER_1);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding the first server triggers the discovery event.
  EXPECT_EQ(graph->getServicesToServersMap()[TEST_SERVICE].size(), 1);
  EXPECT_EQ(graph->getServicesToClientsMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 1);
  EXPECT_TRUE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // ADDING SECOND SERVER
  createTestServiceServer(TEST_SERVICE, TEST_SERVICE_SERVER_2);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding a second server does not trigger an event.
  EXPECT_EQ(graph->getServicesToServersMap()[TEST_SERVICE].size(), 2);
  EXPECT_EQ(graph->getServicesToClientsMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 1);
  EXPECT_FALSE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING SECOND SERVER
  server_map.erase(TEST_SERVICE_SERVER_2 + "|" + TEST_SERVICE);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing a server that is not the last will not trigger a removal event.
  EXPECT_EQ(graph->getServicesToServersMap()[TEST_SERVICE].size(), 1);
  EXPECT_EQ(graph->getServicesToClientsMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 1);
  EXPECT_FALSE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING FIRST SERVER
  server_map.erase(TEST_SERVICE_SERVER_1 + "|" + TEST_SERVICE);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing the last server triggers a removal event.
  EXPECT_EQ(graph->getServicesToServersMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToClientsMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 0);
  EXPECT_FALSE(service_discovered);
  EXPECT_TRUE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  /////////////
  // CLIENTS //
  /////////////

  // ADD FIRST CLIENT
  createTestServiceClient(TEST_SERVICE, TEST_SERVICE_CLIENT_1);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding the first client does not trigger a discovery event.
  EXPECT_EQ(graph->getServicesToServersMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToClientsMap()[TEST_SERVICE].size(), 1);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 0);
  EXPECT_FALSE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // ADDING SECOND CLIENT
  createTestServiceClient(TEST_SERVICE, TEST_SERVICE_CLIENT_2);
  IpcGraphTest::sleepLongEnoughToSync();

  // Adding a second client does not trigger an event.
  EXPECT_EQ(graph->getServicesToServersMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToClientsMap()[TEST_SERVICE].size(), 2);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 0);
  EXPECT_FALSE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING SECOND CLIENT
  client_map.erase(TEST_SERVICE_CLIENT_2 + "|" + TEST_SERVICE);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing a client that is not the last will not trigger a removal event.
  EXPECT_EQ(graph->getServicesToServersMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToClientsMap()[TEST_SERVICE].size(), 1);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 0);
  EXPECT_FALSE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();

  // REMOVING FIRST CLIENT
  client_map.erase(TEST_SERVICE_CLIENT_1 + "|" + TEST_SERVICE);
  IpcGraphTest::sleepLongEnoughToSync();

  // Removing the last client does not trigger a removal event.
  EXPECT_EQ(graph->getServicesToServersMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToClientsMap().count(TEST_SERVICE), 0);
  EXPECT_EQ(graph->getServicesToTypesMap().count(TEST_SERVICE), 0);
  EXPECT_FALSE(service_discovered);
  EXPECT_FALSE(service_removed);
  EXPECT_TRUE(graph_updated);
  reset();
}

TEST_F(IpcGraphTest, GetTopicTypeInfo) {
  bool topic_discovered = false;

  callbacks.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == TEST_TOPIC) {
      topic_discovered = true;
    }
  };

  startIpcGraph();

  createTestPublisher(TEST_TOPIC);

  IpcGraphTest::sleepLongEnoughToSync();

  EXPECT_TRUE(topic_discovered);

  auto type_info = graph->getTopicTypeInfo(TEST_TOPIC);
  EXPECT_TRUE(type_info.has_value());
  EXPECT_EQ(type_info->name, "heph.types.proto.DummyType");
}

TEST_F(IpcGraphTest, GetTopicListString) {
  bool topic_discovered = false;

  callbacks.topic_discovery_cb = [&](const std::string& topic, const heph::serdes::TypeInfo&) {
    if (topic == TEST_TOPIC) {
      topic_discovered = true;
    }
  };

  startIpcGraph();

  createTestPublisher(TEST_TOPIC);

  IpcGraphTest::sleepLongEnoughToSync();

  EXPECT_TRUE(topic_discovered);
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
\
