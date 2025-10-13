//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/ipc_graph.h"

#include <algorithm>
#include <cstddef>
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

#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/telemetry/log.h"

namespace heph::ipc::zenoh {

IpcGraph::IpcGraph(IpcGraphConfig config, IpcGraphCallbacks&& callbacks)
  : config_(std::move(config)), callbacks_(std::move(callbacks)) {
  heph::log(heph::INFO, "[IPC Graph] - Initializing...");

  if (config_.session == nullptr) {
    heph::log(heph::ERROR, "[IPC Graph] - Session is null");
    return;
  }

  heph::log(heph::INFO, "[IPC Graph] - Initialized");
}

void IpcGraph::start() {
  const absl::MutexLock lock(&mutex_);

  heph::log(heph::INFO, "[IPC Graph] - Starting...");

  topic_db_ = ipc::createZenohTopicDatabase(config_.session);

  discovery_ = std::make_unique<ipc::zenoh::EndpointDiscovery>(
      config_.session, TopicFilter::create(),
      [this](const ipc::zenoh::EndpointInfo& info) { endPointInfoUpdateCallback(info); });

  heph::log(heph::INFO, "[IPC Graph] - ONLINE");
}

void IpcGraph::stop() {
  heph::log(heph::INFO, "[IPC Graph] - Stopping...");
  {
    const absl::MutexLock lock(&mutex_);

    topic_db_.reset();

    callbacks_.topic_discovery_cb = nullptr;
    callbacks_.topic_removal_cb = nullptr;
    callbacks_.graph_update_cb = nullptr;
  }

  discovery_.reset();

  heph::log(heph::INFO, "[IPC Graph] - OFFLINE");
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

void IpcGraph::endPointInfoUpdateCallback(const ipc::zenoh::EndpointInfo& info) {
  const absl::MutexLock lock(&mutex_);
  ipc::zenoh::printEndpointInfo(info);

  bool graph_updated = false;

  switch (info.type) {
    case ipc::zenoh::EndpointType::SERVICE_SERVER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          if (addServiceServerEndpoint(info)) {
            graph_updated = true;
          }
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeServiceServerEndpoint(info);
          graph_updated = true;
          break;
      }
      break;
    case ipc::zenoh::EndpointType::SERVICE_CLIENT:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addServiceClientEndPoint(info);
          graph_updated = true;
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeServiceClientEndPoint(info);
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
          if (addPublisherEndpoint(info)) {
            graph_updated = true;
          }
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removePublisherEndpoint(info);
          graph_updated = true;
          break;
      }
      break;
    case ipc::zenoh::EndpointType::SUBSCRIBER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          if (addSubscriberEndpoint(info)) {
            graph_updated = true;
          }
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeSubscriberEndpoint(info);
          graph_updated = true;
          break;
      }
      break;
  }

  if (graph_updated) {
    if (callbacks_.graph_update_cb) {
      callbacks_.graph_update_cb(info, state_);
    }
  }
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
  if (callbacks_.graph_update_cb) {
    const ipc::zenoh::EndpointInfo info{};
    callbacks_.graph_update_cb(info, state_);
  }
}

bool IpcGraph::addPublisherEndpoint(const ipc::zenoh::EndpointInfo& info) {  // NOLINT
  if (!addTopic(info.topic)) {
    // TODO: This can happen if type retrieval fails. We might want to consider retrying later, since
    // we will not get another liveliness event for the same publisher and currently will never re-register
    // this topic (unless the publisher is restarted or another publisher is added).
    return false;
  }

  state_.topic_to_publishers_map[info.topic].push_back(info.session_id);
  return true;
}

void IpcGraph::removePublisherEndpoint(const ipc::zenoh::EndpointInfo& info) {
  auto& publishers = state_.topic_to_publishers_map[info.topic];
  std::erase(publishers, info.session_id);

  // If there are publishers left for this topic, no cleanup is needed.
  if (!publishers.empty()) {
    return;
  }

  // If the last publisher is removed, remove the map entry.
  state_.topic_to_publishers_map.erase(info.topic);

  if (config_.track_topics_based_on_subscribers) {
    // Cleanup the topic if no publishers or subscribers are left.
    if (!topicHasAnyEndpoints(info.topic)) {
      removeTopic(info.topic);
    }
  } else {
    // If we are not tracking topics based on subscribers, we can remove the topic immediately.
    removeTopic(info.topic);
  }
}

bool IpcGraph::addSubscriberEndpoint(const ipc::zenoh::EndpointInfo& info) {  // NOLINT
  if (config_.track_topics_based_on_subscribers) {
    if (!addTopic(info.topic)) {
      // TODO: This can happen if type retrieval fails. We might want to consider retrying later, since
      // we will not get another liveliness event for the same publisher and currently will never re-register
      // this topic (unless the publisher is restarted or another publisher is added).
      return false;
    }
  }

  state_.topic_to_subscribers_map[info.topic].push_back(info.session_id);
  return true;
}

void IpcGraph::removeSubscriberEndpoint(const ipc::zenoh::EndpointInfo& info) {
  auto& subscribers = state_.topic_to_subscribers_map[info.topic];
  std::erase(subscribers, info.session_id);

  // If there are subscribers left for this topic, no cleanup is needed.
  if (!subscribers.empty()) {
    return;
  }

  // If the last subscriber is removed, remove the map entry.
  state_.topic_to_subscribers_map.erase(info.topic);

  if (config_.track_topics_based_on_subscribers) {
    // Cleanup the topic if no publishers or subscribers are left.
    if (!topicHasAnyEndpoints(info.topic)) {
      removeTopic(info.topic);
    }
  }
}

bool IpcGraph::topicHasAnyEndpoints(const std::string& topic) const {  // NOLINT
  auto pub_it = state_.topic_to_publishers_map.find(topic);
  auto sub_it = state_.topic_to_subscribers_map.find(topic);

  return (pub_it != state_.topic_to_publishers_map.end() && !pub_it->second.empty()) ||
         (sub_it != state_.topic_to_subscribers_map.end() && !sub_it->second.empty());
}

bool IpcGraph::addTopic(const std::string& topic_name) {  // NOLINT
  if (hasTopic(topic_name)) {
    return true;
  }

  auto type_info = topic_db_->getTypeInfo(topic_name);
  if (!type_info.has_value()) {
    heph::log(heph::ERROR, "[IPC Graph] - Could not retrieve type info for topic", "topic", topic_name);
    return false;
  }

  state_.topics_to_types_map[topic_name] = type_info->name;

  if (callbacks_.topic_discovery_cb) {
    callbacks_.topic_discovery_cb(topic_name, type_info.value());
  }

  return true;
}

void IpcGraph::removeTopic(const std::string& topic_name) {
  state_.topics_to_types_map.erase(topic_name);
  state_.topic_to_publishers_map.erase(topic_name);
  state_.topic_to_subscribers_map.erase(topic_name);

  if (callbacks_.topic_removal_cb) {
    callbacks_.topic_removal_cb(topic_name);
  }
}

bool IpcGraph::hasTopic(const std::string& topic_name) const {  // NOLINT
  return state_.topics_to_types_map.contains(topic_name);
}

bool IpcGraph::addServiceServerEndpoint(const ipc::zenoh::EndpointInfo& info) {  // NOLINT
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

void IpcGraph::removeServiceServerEndpoint(const ipc::zenoh::EndpointInfo& info) {
  auto& servers = state_.services_to_server_map[info.topic];
  std::erase(servers, info.session_id);

  // If no more servers are available, remove the service altogether.
  if (servers.empty()) {
    state_.services_to_server_map.erase(info.topic);
    removeService(info.topic);
  }
}

bool IpcGraph::hasServiceServerEndPoint(const std::string& service_name) const {  // NOLINT
  // Any server means the service is offered.
  // Note: multiple identical service servers should not exists, but we do not enforce this here.
  return state_.services_to_server_map.contains(service_name);
}

void IpcGraph::addServiceClientEndPoint(const ipc::zenoh::EndpointInfo& info) {
  // We are not tracking services based on clients, but solely on servers.
  // Therefore, we do not need to add the service here.

  state_.services_to_client_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removeServiceClientEndPoint(const ipc::zenoh::EndpointInfo& info) {
  auto& clients = state_.services_to_client_map[info.topic];
  std::erase(clients, info.session_id);

  // If no more clients are available, clean up, but DO NOT remove the service.
  if (clients.empty()) {
    state_.services_to_client_map.erase(info.topic);
  }
}

bool IpcGraph::addService(const std::string& service_name) {  // NOLINT
  if (hasService(service_name)) {
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

  if (callbacks_.service_discovery_cb) {
    callbacks_.service_discovery_cb(service_name, service_type_info.value());
  }

  return true;
}

void IpcGraph::removeService(const std::string& service_name) {
  state_.services_to_types_map.erase(service_name);
  state_.services_to_server_map.erase(service_name);
  state_.services_to_client_map.erase(service_name);

  if (callbacks_.service_removal_cb) {
    callbacks_.service_removal_cb(service_name);
  }
}

bool IpcGraph::hasService(const std::string& service_name) const {  // NOLINT
  return state_.services_to_types_map.contains(service_name);
}

void IpcGraphState::printIpcGraphState() const {
  std::stringstream ss;

  ss << "[IPC Graph]\n";

  ss << "\n  == EDGES ==\n\n";

  const auto max_name_length = [](const auto& map) -> std::size_t {
    if (map.empty()) {
      return 0;
    }
    return std::max_element(map.begin(), map.end(),
                            [](const auto& a, const auto& b) { return a.first.size() < b.first.size(); })
        ->first.size();
  };

  const std::size_t longest_name =
      std::max(max_name_length(topics_to_types_map), max_name_length(services_to_types_map));

  if (!topics_to_types_map.empty()) {
    for (const auto& [topic, type] : topics_to_types_map) {
      ss << "    TOPIC:   '" << topic << "'" << std::string(longest_name - topic.size(), ' ') << " [" << type
         << "]\n";
    }
  }

  ss << "\n";

  if (!services_to_types_map.empty()) {
    for (const auto& [srv, srv_types] : services_to_types_map) {
      ss << "    SERVICE: '" << srv << "'" << std::string(longest_name - srv.size(), ' ') << " ["
         << srv_types.first << "/" << srv_types.second << "]\n";
    }
  }

  ss << "\n  == ENDPOINTS ==\n\n";

  if (!topic_to_publishers_map.empty()) {
    for (const auto& [topic, publishers] : topic_to_publishers_map) {
      ss << "    [";
      for (auto it = publishers.begin(); it != publishers.end(); ++it) {
        ss << *it;
        if (std::next(it) != publishers.end()) {
          ss << ", ";
        }
      }
      ss << "] --PUBLISH--> '" << topic << "'\n";
    }
  }

  ss << "\n";

  if (!topic_to_subscribers_map.empty()) {
    for (const auto& [topic, subscribers] : topic_to_subscribers_map) {
      ss << "    [";
      for (auto it = subscribers.begin(); it != subscribers.end(); ++it) {
        ss << *it;
        if (std::next(it) != subscribers.end()) {
          ss << ", ";
        }
      }
      ss << "] --SUBSCRIBE--> '" << topic << "'\n";
    }
  }

  ss << "\n";

  if (!services_to_server_map.empty()) {
    for (const auto& [srv, nodes] : services_to_server_map) {
      ss << "    [";
      for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        ss << *it;
        if (std::next(it) != nodes.end()) {
          ss << ", ";
        }
      }
      ss << "] --SERVE--> '" << srv << "'\n";
    }
  }

  ss << "\n";

  if (!services_to_client_map.empty()) {
    for (const auto& [srv, nodes] : services_to_client_map) {
      ss << "    [";
      for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        ss << *it;
        if (std::next(it) != nodes.end()) {
          ss << ", ";
        }
      }
      ss << "] --CALL--> '" << srv << "'\n";
    }
  }

  ss << "\n";

  fmt::println("\n{}", ss.str());
}

[[nodiscard]] auto IpcGraphState::checkConsistency() const -> bool {
  // Check that every publisher has a corresponding topic to type entry
  for (const auto& [topic, publishers] : topic_to_publishers_map) {
    if (!topics_to_types_map.contains(topic)) {
      return false;
    }
  }

  // Check that every topic to type entry has at least one publisher or subscriber
  for (const auto& [topic, type] : topics_to_types_map) {
    if ((!topic_to_publishers_map.contains(topic) || topic_to_publishers_map.at(topic).empty()) &&
        (!topic_to_subscribers_map.contains(topic) || topic_to_subscribers_map.at(topic).empty())) {
      return false;
    }
  }

  // Check that every service server has a corresponding service to types entry
  for (const auto& [service, servers] : services_to_server_map) {
    if (!services_to_types_map.contains(service)) {
      return false;
    }
  }

  // Check that every service to types entry has at least one service server
  if (!std::ranges::all_of(services_to_types_map, [this](const auto& pair) {
        const auto& service = pair.first;
        return services_to_server_map.contains(service) && !services_to_server_map.at(service).empty();
      })) {
    return false;
  }

  return true;
}

}  // namespace heph::ipc::zenoh
