//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/ipc_graph.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <absl/synchronization/mutex.h>
#include <fmt/base.h>
#include <fmt/core.h>
#include <hephaestus/ipc/topic.h>
#include <hephaestus/ipc/topic_database.h>
#include <hephaestus/ipc/zenoh/liveliness.h>
#include <hephaestus/serdes/type_info.h>
#include <hephaestus/telemetry/log.h>

namespace heph::ws {

IpcGraph::IpcGraph(IpcGraphConfig config) : config_(std::move(config)) {
}

void IpcGraph::start() {
  const absl::MutexLock lock(&mutex_);

  fmt::println("[IPC Graph] - Starting...");

  topic_db_ = ipc::createZenohTopicDatabase(config_.session);

  discovery_ = std::make_unique<ipc::zenoh::EndpointDiscovery>(
      config_.session, ipc::TopicConfig{ "**" },
      [this](const ipc::zenoh::EndpointInfo& info) { callback_EndPointInfoUpdate(info); });

  fmt::println("[IPC Graph] - ONLINE");
}

void IpcGraph::stop() {
  fmt::println("[IPC Graph] - Stopping...");
  {
    const absl::MutexLock lock(&mutex_);

    topic_db_.reset();

    config_.topic_discovery_cb = nullptr;
    config_.topic_removal_cb = nullptr;
    config_.graph_update_cb = nullptr;
  }

  discovery_.reset();

  fmt::println("[IPC Graph] - OFFLINE");
}

auto IpcGraph::getTopicTypeInfo(const std::string& topic) const -> std::optional<serdes::TypeInfo> {
  const absl::MutexLock lock(&mutex_);
  return topic_db_->getTypeInfo(topic);
}

auto IpcGraph::getServiceTypeInfo(const std::string& service_name) const
    -> std::optional<serdes::ServiceTypeInfo> {
  const absl::MutexLock lock(&mutex_);
  return topic_db_->getServiceTypeInfo(service_name);
}

void IpcGraph::callback_EndPointInfoUpdate(const ipc::zenoh::EndpointInfo& info) {
  const absl::MutexLock lock(&mutex_);
  ipc::zenoh::printEndpointInfo(info);

  bool graph_updated = false;

  switch (info.type) {
    case ipc::zenoh::EndpointType::SERVICE_SERVER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          if (addServiceServer(info)) {
            graph_updated = true;
          }
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
          if (addPublisher(info)) {
            graph_updated = true;
          }
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

auto IpcGraph::getTopicListString() -> std::string {
  const absl::MutexLock lock(&mutex_);
  std::stringstream result;
  size_t max_topic_length = 0;
  size_t max_type_length = 0;

  // Find the maximum lengths for alignment
  for (const auto& [topic, type] : state_.topics_to_types_map) {
    max_topic_length = std::max(topic.length(), max_topic_length);
    max_type_length = std::max(type.length(), max_type_length);
  }

  // Create the formatted string
  for (const auto& [topic, type] : state_.topics_to_types_map) {
    result << " - " << std::left << std::setw(static_cast<int>(max_topic_length)) << topic
           << "\tType: " << std::left << std::setw(static_cast<int>(max_type_length)) << type << "\n";
  }

  return result.str();
}

auto IpcGraph::getTopicsToTypeMap() const -> TopicsToTypeMap {
  const absl::MutexLock lock(&mutex_);
  return state_.topics_to_types_map;
}

auto IpcGraph::getServicesToTypesMap() const -> TopicsToServiceTypesMap {
  const absl::MutexLock lock(&mutex_);
  return state_.services_to_types_map;
}

auto IpcGraph::getServicesToServersMap() const -> TopicToSessionIdMap {
  const absl::MutexLock lock(&mutex_);
  return state_.services_to_server_map;
}

auto IpcGraph::getServicesToClientsMap() const -> TopicToSessionIdMap {
  const absl::MutexLock lock(&mutex_);
  return state_.services_to_client_map;
}

auto IpcGraph::getTopicToSubscribersMap() const -> TopicToSessionIdMap {
  const absl::MutexLock lock(&mutex_);
  return state_.topic_to_subscribers_map;
}

auto IpcGraph::getTopicToPublishersMap() const -> TopicToSessionIdMap {
  const absl::MutexLock lock(&mutex_);
  return state_.topic_to_publishers_map;
}

void IpcGraph::refreshConnectionGraph() const {
  const absl::MutexLock lock(&mutex_);
  if (config_.graph_update_cb) {
    const ipc::zenoh::EndpointInfo info{};
    config_.graph_update_cb(info, state_);
  }
}

bool IpcGraph::addPublisher(const ipc::zenoh::EndpointInfo& info) {  // NOLINT
  // A publisher means this topic is actually offered by someone and should be tracked.
  if (!addTopic(info.topic)) {
    // TODO: This can happen if type retrieval fails. We might want to consider retrying later, since
    // we will not get another liveliness event for the same publisher and currently will never re-register
    // this topic (unless the publisher is restarted or another publisher is added).
    return false;
  }

  state_.topic_to_publishers_map[info.topic].push_back(info.session_id);
  return true;
}

void IpcGraph::removePublisher(const ipc::zenoh::EndpointInfo& info) {
  auto& publishers = state_.topic_to_publishers_map[info.topic];
  std::erase(publishers, info.session_id);

  // If the last publisher is removed, remove the topic altogether.
  if (publishers.empty()) {
    state_.topic_to_publishers_map.erase(info.topic);
    removeTopic(info.topic);
  }
}

bool IpcGraph::hasPublisher(const std::string& topic) const {  // NOLINT
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
  std::erase(subscribers, info.session_id);

  // If the last subscriber is removed, clean up, but DO NOT remove the topic.
  if (subscribers.empty()) {
    state_.topic_to_subscribers_map.erase(info.topic);
  }
}

bool IpcGraph::addTopic(const std::string& topic_name) {  // NOLINT
  if (hasTopic(topic_name)) {
    heph::log(heph::WARN, "[IPC Graph] - topic is already known", "topic", topic_name);
    return true;
  }

  auto type_info = topic_db_->getTypeInfo(topic_name);
  if (!type_info.has_value()) {
    heph::log(heph::ERROR, "[IPC Graph] - Could not retrieve type info for topic", "topic", topic_name);
    return false;
  }

  state_.topics_to_types_map[topic_name] = type_info->name;

  if (config_.topic_discovery_cb) {
    config_.topic_discovery_cb(topic_name, type_info.value());
  }

  return true;
}

void IpcGraph::removeTopic(const std::string& topic_name) {
  state_.topics_to_types_map.erase(topic_name);
  state_.topic_to_publishers_map.erase(topic_name);
  state_.topic_to_subscribers_map.erase(topic_name);

  if (config_.topic_removal_cb) {
    config_.topic_removal_cb(topic_name);
  }
}

bool IpcGraph::hasTopic(const std::string& topic_name) const {  // NOLINT
  return state_.topics_to_types_map.find(topic_name) != state_.topics_to_types_map.end();
}

bool IpcGraph::addServiceServer(const ipc::zenoh::EndpointInfo& info) {  // NOLINT
  // A server means this service is actually offered by someone and needs tracking.
  if (!addService(info.topic)) {
    // TODO: This can happen if type retrieval fails. We might want to consider retrying later, since
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
  std::erase(servers, info.session_id);

  // If no more servers are available, remove the service altogether.
  if (servers.empty()) {
    state_.services_to_server_map.erase(info.topic);
    removeService(info.topic);
  }
}

bool IpcGraph::hasServiceServer(const std::string& service_name) const {  // NOLINT
  // Any server means the service is offered.
  // Note: multiple identical service servers should not exists, but we do not enforce this here.
  return state_.services_to_server_map.find(service_name) != state_.services_to_server_map.end();
}

void IpcGraph::addServiceClient(const ipc::zenoh::EndpointInfo& info) {
  // We are not tracking services based on clients, but solely on servers.
  // Therefore, we do not need to add the service here.

  state_.services_to_client_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removeServiceClient(const ipc::zenoh::EndpointInfo& info) {
  auto& clients = state_.services_to_client_map[info.topic];
  std::erase(clients, info.session_id);

  // If no more clients are available, clean up, but DO NOT remove the service.
  if (clients.empty()) {
    state_.services_to_client_map.erase(info.topic);
  }
}

bool IpcGraph::addService(const std::string& service_name) {  // NOLINT
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

bool IpcGraph::hasService(const std::string& service_name) const {  // NOLINT
  return state_.services_to_types_map.find(service_name) != state_.services_to_types_map.end();
}

void IpcGraphState::printIpcGraphState() const {
  std::stringstream ss;

  ss << "[IPC Graph] - State:\n";

  if (!topics_to_types_map.empty()) {
    ss << "\n  TOPICS:\n";
    for (const auto& [topic, type] : topics_to_types_map) {
      ss << "    '" << topic << "' [" << type << "]\n";
    }
  }

  if (!topic_to_publishers_map.empty()) {
    ss << "\n  PUBLISHERS:\n";
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
    ss << "\n  SUBSCRIBERS:\n";
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

  if (!services_to_types_map.empty()) {
    ss << "\n  SERVICES:\n";
    for (const auto& [srv, srv_types] : services_to_types_map) {
      ss << "    '" << srv << "' [" << srv_types.first << "/" << srv_types.second << "]\n";
    }
  }

  if (!services_to_server_map.empty()) {
    ss << "\n  SERVERS:\n";
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
    ss << "\n  CLIENTS:\n";
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

  ss << "\n";

  fmt::print("{}", ss.str());
}

[[nodiscard]] auto IpcGraphState::checkConsistency() const -> bool {
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
  if (!std::ranges::all_of(services_to_types_map, [this](const auto& pair) {
        const auto& service = pair.first;
        return services_to_server_map.find(service) != services_to_server_map.end() &&
               !services_to_server_map.at(service).empty();
      })) {
    return false;
  }

  return true;
}

}  // namespace heph::ws
