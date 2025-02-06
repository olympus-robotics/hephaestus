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

struct IpcGraphConfig {
  ipc::zenoh::SessionPtr session;

  TopicDiscoveryCallback topic_discovery_cb;
  TopicRemovalCallback topic_removal_cb;

  GraphUpdateCallback graph_update_cb;
};

class IpcGraph {
public:
  explicit IpcGraph(const IpcGraphConfig& config);

  void Start();
  void Stop();

  // Access function to query theinternal topic type db.
  [[nodiscard]] std::optional<heph::serdes::TypeInfo> getTopicTypeInfo(const std::string& topic) const;

  // Create a human readable, multi-line, console-optimized list of topics and
  // their types as they are stored in  topics_to_types_map_.
  std::string GetTopicListString();

  TopicsToTypesMap GetTopicsToTypesMap() const;

  TopicsToTypesMap GetServicesToTypesMap() const;

  TopicsToTypesMap GetServicesToNodesMap() const;

  TopicToNodesMap GetTopicToSubscribersMap() const;

  TopicToNodesMap GetTopicToPublishersMap() const;

private:
  void CallbackLivelinessUpdate(const ipc::zenoh::EndpointInfo& info);

private:
  ipc::zenoh::SessionPtr session_;
  std::unique_ptr<ipc::zenoh::EndpointDiscovery> discovery_;

private:
  void addPublisher(const ipc::zenoh::EndpointInfo& info);
  void removePublisher(const ipc::zenoh::EndpointInfo& info);
  bool hasPublisher(const std::string& topic);

  void addSubscriber(const ipc::zenoh::EndpointInfo& info);
  void removeSubscriber(const ipc::zenoh::EndpointInfo& info);

  bool addTopic(const std::string& topic);
  void removeTopic(const std::string& topic);
  bool hasTopic(const std::string& topic_name);

  IpcGraphState state_;

  std::unique_ptr<ipc::ITopicDatabase> topic_db_;

private:
  TopicRemovalCallback topic_removal_cb_;
  TopicDiscoveryCallback topic_discovery_cb_;
};

}  // namespace heph::ws_bridge
