//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {

using TopicsToTypeMap = std::unordered_map<std::string, std::string>;
using TopicsToServiceTypesMap = std::unordered_map<std::string, std::pair<std::string, std::string>>;
using TopicToSessionIdMap = std::unordered_map<std::string, std::vector<std::string>>;

struct IpcGraphState {
  TopicsToTypeMap topics_to_types_map;

  TopicToSessionIdMap topic_to_publishers_map;
  TopicToSessionIdMap topic_to_subscribers_map;

  TopicsToServiceTypesMap services_to_types_map;

  TopicToSessionIdMap services_to_server_map;
  TopicToSessionIdMap services_to_client_map;

  void printIpcGraphState() const;
  [[nodiscard]] auto checkConsistency() const -> bool;
};

struct IpcGraphCallbacks {
  using TopicRemovalCallback = std::function<void(const std::string& /*topic*/)>;
  using TopicDiscoveryCallback =
      std::function<void(const std::string& /*topic*/, const serdes::TypeInfo& /*type_info*/)>;
  using ServiceRemovalCallback = std::function<void(const std::string& /*service_name*/)>;
  using ServiceDiscoveryCallback = std::function<void(const std::string& /*service_name*/,
                                                      const serdes::ServiceTypeInfo& /*service_type_info*/)>;
  using GraphUpdateCallback =
      std::function<void(const ipc::zenoh::EndpointInfo& /*info*/, const IpcGraphState& /*state*/)>;

  TopicDiscoveryCallback topic_discovery_cb;
  TopicRemovalCallback topic_removal_cb;

  ServiceDiscoveryCallback service_discovery_cb;
  ServiceRemovalCallback service_removal_cb;

  GraphUpdateCallback graph_update_cb;
};

struct IpcGraphConfig {
  ipc::zenoh::SessionPtr session;

  bool track_topics_based_on_subscribers = true;
};

class IpcGraph {
public:
  IpcGraph(IpcGraphConfig config, IpcGraphCallbacks&& callbacks);

  void start();
  void stop();

  [[nodiscard]] auto getTopicTypeInfo(const std::string& topic) const -> std::optional<serdes::TypeInfo>;
  [[nodiscard]] auto getServiceTypeInfo(const std::string& service_name) const
      -> std::optional<serdes::ServiceTypeInfo>;

  [[nodiscard]] auto getTopicsToTypeMap() const -> TopicsToTypeMap;
  [[nodiscard]] auto getServicesToTypesMap() const -> TopicsToServiceTypesMap;
  [[nodiscard]] auto getServicesToServersMap() const -> TopicToSessionIdMap;
  [[nodiscard]] auto getServicesToClientsMap() const -> TopicToSessionIdMap;
  [[nodiscard]] auto getTopicToSubscribersMap() const -> TopicToSessionIdMap;
  [[nodiscard]] auto getTopicToPublishersMap() const -> TopicToSessionIdMap;

  void refreshConnectionGraph() const;

  void endPointInfoUpdateCallback(const ipc::zenoh::EndpointInfo& info);

private:
  // Note: The ABSL EXCLUSIVE_LOCKS_REQUIRED(mutex_) annotations don't seem to play nice with trailing return
  // types.
  // NOLINTBEGIN(modernize-use-trailing-return-type)

  // Publishers
  /////////////
  [[nodiscard]] bool addPublisherEndpoint(const ipc::zenoh::EndpointInfo& info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removePublisherEndpoint(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Subscribers
  //////////////
  [[nodiscard]] bool addSubscriberEndpoint(const ipc::zenoh::EndpointInfo& info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeSubscriberEndpoint(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  [[nodiscard]] bool topicHasAnyEndpoints(const std::string& topic) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Topics
  /////////
  // The functions below are used to track topics and their types.
  // Only publishers contribute to this tracking, subscribers are ignored.
  [[nodiscard]] bool addTopic(const std::string& topic_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeTopic(const std::string& topic_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasTopic(const std::string& topic_name) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Service Servers
  //////////////////
  [[nodiscard]] bool addServiceServerEndpoint(const ipc::zenoh::EndpointInfo& info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeServiceServerEndpoint(const ipc::zenoh::EndpointInfo& info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasServiceServerEndPoint(const std::string& service_name) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Service Clients
  //////////////////
  void addServiceClientEndPoint(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeServiceClientEndPoint(const ipc::zenoh::EndpointInfo& info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Services
  ///////////
  // The functions below are used to track services and their types.
  // Only service servers contribute to this tracking, clients are ignored.
  [[nodiscard]] bool addService(const std::string& service_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeService(const std::string& service_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasService(const std::string& service_name) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // NOLINTEND(modernize-use-trailing-return-type)

private:
  mutable absl::Mutex mutex_;

  IpcGraphConfig config_;
  IpcGraphCallbacks callbacks_;

  std::unique_ptr<ipc::zenoh::EndpointDiscovery> discovery_;

  IpcGraphState state_ ABSL_GUARDED_BY(mutex_);

  std::unique_ptr<ipc::ITopicDatabase> topic_db_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace heph::ipc::zenoh
