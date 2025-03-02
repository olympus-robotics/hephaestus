//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
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

namespace heph::ws {

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

using TopicRemovalCallback = std::function<void(const std::string& /*topic*/)>;
using TopicDiscoveryCallback =
    std::function<void(const std::string& /*topic*/, const serdes::TypeInfo& /*type_info*/)>;

using ServiceRemovalCallback = std::function<void(const std::string& /*service_name*/)>;
using ServiceDiscoveryCallback = std::function<void(const std::string& /*service_name*/,
                                                    const serdes::ServiceTypeInfo& /*service_type_info*/)>;

using GraphUpdateCallback =
    std::function<void(const ipc::zenoh::EndpointInfo& /*info*/, IpcGraphState /*state*/)>;

struct IpcGraphConfig {
  ipc::zenoh::SessionPtr session;

  TopicDiscoveryCallback topic_discovery_cb;
  TopicRemovalCallback topic_removal_cb;

  ServiceDiscoveryCallback service_discovery_cb;
  ServiceRemovalCallback service_removal_cb;

  GraphUpdateCallback graph_update_cb;
};

class IpcGraph {
public:
  explicit IpcGraph(IpcGraphConfig config);

  void start();
  void stop();

  [[nodiscard]] auto getTopicTypeInfo(const std::string& topic) const -> std::optional<serdes::TypeInfo>;
  [[nodiscard]] auto getServiceTypeInfo(const std::string& service_name) const
      -> std::optional<serdes::ServiceTypeInfo>;

  // Create a human readable, multi-line, console-optimized list of topics and
  // their types as they are stored in  topics_to_types_map_.
  [[nodiscard]] auto getTopicListString() -> std::string;

  [[nodiscard]] auto getTopicsToTypeMap() const -> TopicsToTypeMap;
  [[nodiscard]] auto getServicesToTypesMap() const -> TopicsToServiceTypesMap;
  [[nodiscard]] auto getServicesToServersMap() const -> TopicToSessionIdMap;
  [[nodiscard]] auto getServicesToClientsMap() const -> TopicToSessionIdMap;
  [[nodiscard]] auto getTopicToSubscribersMap() const -> TopicToSessionIdMap;
  [[nodiscard]] auto getTopicToPublishersMap() const -> TopicToSessionIdMap;

  void refreshConnectionGraph() const;

  // NOLINTNEXTLINE(readability-identifier-naming)
  void callback_EndPointInfoUpdate(const ipc::zenoh::EndpointInfo& info);

private:
  // Note: The ABSL EXCLUSIVE_LOCKS_REQUIRED(mutex_) annotations don't seem to play nice with trailing return
  // types.
  // NOLINTBEGIN(modernize-use-trailing-return-type)

  // Publisher / Subscriber tracking
  //////////////////////////////////
  [[nodiscard]] bool addPublisher(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removePublisher(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasPublisher(const std::string& topic) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void addSubscriber(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeSubscriber(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Topic Tracking
  /////////////////
  // The functions below are used to track topics and their types.
  // Only publishers contribute to this tracking, subscribers are ignored.

  [[nodiscard]] bool addTopic(const std::string& topic_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeTopic(const std::string& topic_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasTopic(const std::string& topic_name) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Service Server / Client tracking
  ///////////////////////////////////

  [[nodiscard]] bool addServiceServer(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeServiceServer(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasServiceServer(const std::string& service_name) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void addServiceClient(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeServiceClient(const ipc::zenoh::EndpointInfo& info) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Services Tracking
  ////////////////////
  // The functions below are used to track services and their types.
  // Only service servers contribute to this tracking, clients are ignored.

  [[nodiscard]] bool addService(const std::string& service_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void removeService(const std::string& service_name) ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  [[nodiscard]] bool hasService(const std::string& service_name) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // NOLINTEND(modernize-use-trailing-return-type)

private:
  mutable absl::Mutex mutex_;

  IpcGraphConfig config_;

  std::unique_ptr<ipc::zenoh::EndpointDiscovery> discovery_;

  IpcGraphState state_ ABSL_GUARDED_BY(mutex_);

  std::unique_ptr<ipc::ITopicDatabase> topic_db_ ABSL_GUARDED_BY(mutex_);

  TopicRemovalCallback topic_removal_cb_;
  TopicDiscoveryCallback topic_discovery_cb_;
};

}  // namespace heph::ws
