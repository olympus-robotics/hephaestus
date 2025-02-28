//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/ipc_graph.h"

#include <memory>
#include <string>
#include <vector>

#include <absl/log/check.h>
#include <absl/synchronization/mutex.h>
#include <fmt/base.h>
#include <fmt/core.h>
#include <hephaestus/ipc/zenoh/liveliness.h>
#include <hephaestus/telemetry/log.h>

namespace heph::ws_bridge {

IpcGraph::IpcGraph(const IpcGraphConfig& config) : config_(config) {
}

IpcGraph::~IpcGraph() {
  stop();
}

void IpcGraph::start() {
  absl::MutexLock lock(&mutex_);

  fmt::println("[IPC Graph] - Starting...");

  topic_db_ = ipc::createZenohTopicDatabase(config_.session);

  discovery_ = std::make_unique<ipc::zenoh::EndpointDiscovery>(
      config_.session, ipc::TopicConfig{ "**" },
      [this](const ipc::zenoh::EndpointInfo& info) { callback__EndPointInfoUpdate(info); });

  fmt::println("[IPC Graph] - ONLINE");
}

void IpcGraph::stop() {
  absl::MutexLock lock(&mutex_);
  fmt::println("[IPC Graph] - Stopping...");

  topic_db_.reset();

  config_.topic_discovery_cb = nullptr;
  config_.topic_removal_cb = nullptr;
  config_.graph_update_cb = nullptr;

  discovery_.reset();

  fmt::println("[IPC Graph] - OFFLINE");
}

std::optional<serdes::TypeInfo> IpcGraph::getTopicTypeInfo(const std::string& topic) const {
  absl::MutexLock lock(&mutex_);
  return topic_db_->getTypeInfo(topic);
}

[[nodiscard]] std::optional<serdes::ServiceTypeInfo>
IpcGraph::getServiceTypeInfo(const std::string& service_name) const {
  absl::MutexLock lock(&mutex_);
  return topic_db_->getServiceTypeInfo(service_name);
}

void IpcGraph::callback__EndPointInfoUpdate(const ipc::zenoh::EndpointInfo& info) {
  absl::MutexLock lock(&mutex_);
  ipc::zenoh::printEndpointInfo(info);

  bool graph_updated = false;

  switch (info.type) {
    case ipc::zenoh::EndpointType::SERVICE_SERVER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addServiceServer(info);
          graph_updated = true;
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeServiceServer(info);
          graph_updated = true;
          break;
      }
      break;
    case ipc::zenoh::EndpointType::SERVICE_CLIENT:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addServiceClient(info);
          graph_updated = true;
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeServiceClient(info);
          graph_updated = true;
          break;
      }
      break;
    case ipc::zenoh::EndpointType::ACTION_SERVER:
      // TODO: Implement action server handling
      break;
    case ipc::zenoh::EndpointType::PUBLISHER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addPublisher(info);
          graph_updated = true;
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removePublisher(info);
          graph_updated = true;
          break;
      }
      break;
    case ipc::zenoh::EndpointType::SUBSCRIBER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addSubscriber(info);
          graph_updated = true;
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeSubscriber(info);
          graph_updated = true;
          break;
      }
      break;
  }

  if (graph_updated) {
    fmt::print("[IPC Graph] - Graph updated!\n");
    if (config_.graph_update_cb) {
      config_.graph_update_cb(info, state_);
    }
  }
}

std::string IpcGraph::getTopicListString() {
  absl::MutexLock lock(&mutex_);
  std::stringstream result;
  size_t max_topic_length = 0;
  size_t max_type_length = 0;

  // Find the maximum lengths for alignment
  for (const auto& [topic, type] : state_.topics_to_types_map) {
    if (topic.length() > max_topic_length) {
      max_topic_length = topic.length();
    }
    if (type.length() > max_type_length) {
      max_type_length = type.length();
    }
  }

  // Create the formatted string
  for (const auto& [topic, type] : state_.topics_to_types_map) {
    result << " - " << std::left << std::setw(static_cast<int>(max_topic_length)) << topic
           << "\tType: " << std::left << std::setw(static_cast<int>(max_type_length)) << type << "\n";
  }

  return result.str();
}

TopicsToTypeMap IpcGraph::getTopicsToTypeMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.topics_to_types_map;
}

TopicsToServiceTypesMap IpcGraph::getServicesToTypesMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.services_to_types_map;
}

TopicToSessionIdMap IpcGraph::getServicesToServersMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.services_to_server_map;
}

TopicToSessionIdMap IpcGraph::getServicesToClientsMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.services_to_client_map;
}

TopicToSessionIdMap IpcGraph::getTopicToSubscribersMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.topic_to_subscribers_map;
}

TopicToSessionIdMap IpcGraph::getTopicToPublishersMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.topic_to_publishers_map;
}

void IpcGraph::refreshConnectionGraph() const {
  absl::MutexLock lock(&mutex_);
  if (config_.graph_update_cb) {
    ipc::zenoh::EndpointInfo info;
    config_.graph_update_cb(info, state_);
  }
}

void IpcGraph::addPublisher(const ipc::zenoh::EndpointInfo& info) {
  // A publisher means this topic is actually offered by someone and should be tracked.
  if (!addTopic(info.topic)) {
    // TODO(mfehr): This can happen if type retrieval fails. We might want to consider retrying later, since
    // we will not get another liveliness event for the same publisher and currently will never re-register
    // this topic (unless the publisher is restarted or another publisher is added).
    return;
  }

  state_.topic_to_publishers_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removePublisher(const ipc::zenoh::EndpointInfo& info) {
  auto& publishers = state_.topic_to_publishers_map[info.topic];
  publishers.erase(std::remove(publishers.begin(), publishers.end(), info.session_id), publishers.end());

  // If the last publisher is removed, remove the topic altogether.
  if (publishers.empty()) {
    state_.topic_to_publishers_map.erase(info.topic);
    removeTopic(info.topic);
  }
}

bool IpcGraph::hasPublisher(const std::string& topic) const {
  // Any publisher means the topic is offered.
  return state_.topic_to_publishers_map.find(topic) != state_.topic_to_publishers_map.end();
}

void IpcGraph::addSubscriber(const ipc::zenoh::EndpointInfo& info) {
  // We are not tracking topics based on subscribers, but solely on publishers.
  // Therefore, we do not need to add the topic here.

  state_.topic_to_subscribers_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removeSubscriber(const ipc::zenoh::EndpointInfo& info) {
  auto& subscribers = state_.topic_to_subscribers_map[info.topic];
  subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), info.session_id), subscribers.end());

  // If the last subscriber is removed, clean up, but DO NOT remove the topic.
  if (subscribers.empty()) {
    state_.topic_to_subscribers_map.erase(info.topic);
  }
}

bool IpcGraph::addTopic(const std::string& topic) {
  if (hasTopic(topic)) {
    heph::log(heph::WARN, "[IPC Graph] - topic is already known", "topic", topic);
    return true;
  }

  auto type_info = topic_db_->getTypeInfo(topic);
  if (!type_info.has_value()) {
    heph::log(heph::ERROR, "[IPC Graph] - Could not retrieve type info for topic", "topic", topic);
    return false;
  }

  state_.topics_to_types_map[topic] = type_info->name;

  if (config_.topic_discovery_cb) {
    config_.topic_discovery_cb(topic, type_info.value());
  }

  return true;
}

void IpcGraph::removeTopic(const std::string& topic) {
  state_.topics_to_types_map.erase(topic);
  state_.topic_to_publishers_map.erase(topic);
  state_.topic_to_subscribers_map.erase(topic);

  if (config_.topic_removal_cb) {
    config_.topic_removal_cb(topic);
  }
}

bool IpcGraph::hasTopic(const std::string& topic_name) const {
  return state_.topics_to_types_map.find(topic_name) != state_.topics_to_types_map.end();
}

bool IpcGraph::addServiceServer(const ipc::zenoh::EndpointInfo& info) {
  // A server means this service is actually offered by someone and needs tracking.
  if (!addService(info.topic)) {
    // TODO(mfehr): This can happen if type retrieval fails. We might want to consider retrying later, since
    // we will not get another liveliness event for the same service and currently will never re-register this
    // service (unless the server is restarted or another server is added).

    // Note: multiple identical service servers should not exists, but we do not enforce this here.
    return false;
  }

  state_.services_to_server_map[info.topic].push_back(info.session_id);
  return true;
}

void IpcGraph::removeServiceServer(const ipc::zenoh::EndpointInfo& info) {
  auto& servers = state_.services_to_server_map[info.topic];
  servers.erase(std::remove(servers.begin(), servers.end(), info.session_id), servers.end());

  // If no more servers are available, remove the service altogether.
  if (servers.empty()) {
    state_.services_to_server_map.erase(info.topic);
    removeService(info.topic);
  }
}

bool IpcGraph::hasServiceServer(const std::string& service_name) const {
  // Any server means the service is offered.
  // Note: multiple identical service servers should not exists, but we do not enforce this here.
  return state_.services_to_server_map.find(service_name) != state_.services_to_server_map.end();
}

bool IpcGraph::addServiceClient(const ipc::zenoh::EndpointInfo& info) {
  // We are not tracking services based on clients, but solely on servers.
  // Therefore, we do not need to add the service here.

  state_.services_to_client_map[info.topic].push_back(info.session_id);
  return true;
}

void IpcGraph::removeServiceClient(const ipc::zenoh::EndpointInfo& info) {
  auto& clients = state_.services_to_client_map[info.topic];
  clients.erase(std::remove(clients.begin(), clients.end(), info.session_id), clients.end());

  // If no more clients are available, clean up, but DO NOT remove the service.
  if (clients.empty()) {
    state_.services_to_client_map.erase(info.topic);
  }
}

bool IpcGraph::addService(const std::string& service_name) {
  if (hasService(service_name)) {
    heph::log(heph::WARN, "[IPC Graph] - service is already known", "service", service_name);
    return true;
  }

  auto service_type_info = topic_db_->getServiceTypeInfo(service_name);
  if (!service_type_info.has_value()) {
    heph::log(heph::ERROR, "[IPC Graph] - Could not retrieve type info for service", "service", service_name);
    return false;
  }

  const auto& request_type_name = service_type_info->request.name;
  const auto& reply_type_name = service_type_info->reply.name;

  state_.services_to_types_map[service_name] = std::make_pair(request_type_name, reply_type_name);

  if (config_.service_discovery_cb) {
    config_.service_discovery_cb(service_name, service_type_info.value());
  }

  return true;
}

void IpcGraph::removeService(const std::string& service_name) {
  state_.services_to_types_map.erase(service_name);
  state_.services_to_server_map.erase(service_name);
  state_.services_to_client_map.erase(service_name);

  if (config_.service_removal_cb) {
    config_.service_removal_cb(service_name);
  }
}

bool IpcGraph::hasService(const std::string& service_name) const {
  return state_.services_to_types_map.find(service_name) != state_.services_to_types_map.end();
}

void IpcGraphState::printIpcGraphState() const {
  std::stringstream ss;

  ss << "[IPC Graph] - State:\n";

  if (!topics_to_types_map.empty()) {
    ss << "\n  Topics:\n";
    for (const auto& [topic, type] : topics_to_types_map) {
      ss << "    '" << topic << "' [" << type << "]\n";
    }
  }

  if (!services_to_types_map.empty()) {
    ss << "\n  Services:\n";
    for (const auto& [srv, srv_types] : services_to_types_map) {
      ss << "    '" << srv << "' [" << srv_types.first << "/" << srv_types.second << "]\n";
    }
  }

  if (!services_to_server_map.empty()) {
    ss << "\n  Service Servers:\n";
    for (const auto& [srv, nodes] : services_to_server_map) {
      ss << "    '" << srv << "' [";
      for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        ss << *it;
        if (std::next(it) != nodes.end()) {
          ss << ", ";
        }
      }
      ss << "]\n";
    }
  }

  if (!services_to_client_map.empty()) {
    ss << "\n  Service Clients:\n";
    for (const auto& [srv, nodes] : services_to_client_map) {
      ss << "    '" << srv << "' [";
      for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        ss << *it;
        if (std::next(it) != nodes.end()) {
          ss << ", ";
        }
      }
      ss << "]\n";
    }
  }

  if (!topic_to_publishers_map.empty()) {
    ss << "\n  Publishers:\n";
    for (const auto& [topic, publishers] : topic_to_publishers_map) {
      ss << "    '" << topic << "' <- [";
      for (auto it = publishers.begin(); it != publishers.end(); ++it) {
        ss << *it;
        if (std::next(it) != publishers.end()) {
          ss << ", ";
        }
    }
      ss << "]\n";
    }
  }

  if (!topic_to_subscribers_map.empty()) {
    ss << "\n  Subscribers:\n";
    for (const auto& [topic, subscribers] : topic_to_subscribers_map) {
      ss << "    '" << topic << "' -> [";
      for (auto it = subscribers.begin(); it != subscribers.end(); ++it) {
        ss << *it;
        if (std::next(it) != subscribers.end()) {
          ss << ", ";
        }
      }
      ss << "]\n";
    }
  }

  ss << "\n";

  fmt::print("{}", ss.str());
}

[[nodiscard]] bool IpcGraphState::checkConsistency() const {
  // Check that every publisher has a corresponding topic to type entry
  for (const auto& [topic, publishers] : topic_to_publishers_map) {
    if (topics_to_types_map.find(topic) == topics_to_types_map.end()) {
      return false;
    }
  }

  // Check that every topic to type entry has at least one publisher
  for (const auto& [topic, type] : topics_to_types_map) {
    if (topic_to_publishers_map.find(topic) == topic_to_publishers_map.end() ||
        topic_to_publishers_map.at(topic).empty()) {
      return false;
    }
  }

  // Check that every service server has a corresponding service to types entry
  for (const auto& [service, servers] : services_to_server_map) {
    if (services_to_types_map.find(service) == services_to_types_map.end()) {
      return false;
    }
  }

  // Check that every service to types entry has at least one service server
  for (const auto& [service, types] : services_to_types_map) {
    if (services_to_server_map.find(service) == services_to_server_map.end() ||
        services_to_server_map.at(service).empty()) {
      return false;
    }
  }

  return true;
}

}  // namespace heph::ws_bridge
