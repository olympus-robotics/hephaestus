//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// #define ZENOHCXX_ZENOHC

#include <absl/synchronization/mutex.h>
#include <hephaestus/ipc/topic_database.h>
#include <hephaestus/ipc/zenoh/liveliness.h>

namespace heph::ws_bridge {

using TopicsToTypesMap = std::unordered_map<std::string, std::string>;
using TopicToNodesMap = std::unordered_map<std::string, std::vector<std::string>>;

struct IpcGraphState {
  TopicsToTypesMap topics_to_types_map;
  TopicsToTypesMap services_to_types_map;
  TopicsToTypesMap services_to_nodes_map;
  TopicToNodesMap topic_to_publishers_map;
  TopicToNodesMap topic_to_subscribers_map;
};

using TopicRemovalCallback = std::function<void(const std::string& /*topic*/)>;
using TopicDiscoveryCallback =
    std::function<void(const std::string& /*topic*/, const heph::serdes::TypeInfo& /*type_info*/)>;
using GraphUpdateCallback = std::function<void(IpcGraphState /*state*/)>;

/**
 * @struct IpcGraphConfig
 * @brief Configuration structure for IPC graph.
 *
 * @note Pub/sub additions / removal events within a single sessions are
 * only triggering a liveliness token update once per topic/typename/endpoint_type
 * This means that multiple pubs / subs of same topic/type within a session will not
 * trigger multiple liveliness updates and with that not multiple callback events.
 * If across sessions, the liveliness token will be updated for each new publisher/subscriber,
 * even if the topic/type is the same as in another session.
 */
struct IpcGraphConfig {
  ipc::zenoh::SessionPtr session;

  TopicDiscoveryCallback topic_discovery_cb;
  TopicRemovalCallback topic_removal_cb;

  GraphUpdateCallback graph_update_cb;
};

/**
 * @class IpcGraph
 * @brief Class to monitor the IPC graph and trigger callbacks in case of changes.
 */
class IpcGraph {
public:
  explicit IpcGraph(const IpcGraphConfig& config);

  ~IpcGraph();

  void start();
  void stop();

  [[nodiscard]] std::optional<heph::serdes::TypeInfo> getTopicTypeInfo(const std::string& topic) const;

  // Create a human readable, multi-line, console-optimized list of topics and
  // their types as they are stored in  topics_to_types_map_.
  std::string getTopicListString();

  TopicsToTypesMap getTopicsToTypesMap() const;
  TopicsToTypesMap getServicesToTypesMap() const;
  TopicsToTypesMap getServicesToNodesMap() const;
  TopicToNodesMap getTopicToSubscribersMap() const;
  TopicToNodesMap getTopicToPublishersMap() const;

  void refreshConnectionGraph() const;

private:
  void addPublisher(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removePublisher(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  bool hasPublisher(const std::string& topic) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void addSubscriber(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeSubscriber(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  bool addTopic(const std::string& topic) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeTopic(const std::string& topic) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  bool hasTopic(const std::string& topic_name) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

private:
  void callback__EndPointInfoUpdate(const ipc::zenoh::EndpointInfo& info);

  mutable absl::Mutex mutex_;

  IpcGraphConfig config_;

  std::unique_ptr<ipc::zenoh::EndpointDiscovery> discovery_;

  IpcGraphState state_ ABSL_GUARDED_BY(mutex_);

  std::unique_ptr<ipc::ITopicDatabase> topic_db_ ABSL_GUARDED_BY(mutex_);

  TopicRemovalCallback topic_removal_cb_;
  TopicDiscoveryCallback topic_discovery_cb_;
};

}  // namespace heph::ws_bridge
