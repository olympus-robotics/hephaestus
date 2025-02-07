#include "hephaestus/websocket_bridge/ipc_graph.h"

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

IpcGraph::IpcGraph(const IpcGraphConfig& config)
  : config_(config), topic_db_(ipc::createZenohTopicDatabase(config.session)) {
}

void IpcGraph::callback__EndPointInfoUpdate(const ipc::zenoh::EndpointInfo& info) {
  ipc::zenoh::printEndpointInfo(info);

  absl::MutexLock lock(&mutex_);
  bool graph_updated = false;

  switch (info.type) {
    case ipc::zenoh::EndpointType::SERVICE_SERVER:
      break;
      // TODO: Implement service server handling
    case ipc::zenoh::EndpointType::SERVICE_CLIENT:
      break;
      // TODO: Implement service client handling
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
      }
      break;
  }

  if (graph_updated) {
    if (config_.graph_update_cb) {
      config_.graph_update_cb(state_);
    }
  }
}

void IpcGraph::start() {
  absl::MutexLock lock(&mutex_);
  discovery_ = std::make_unique<ipc::zenoh::EndpointDiscovery>(
      config_.session, ipc::TopicConfig{ "**" },
      [this](const ipc::zenoh::EndpointInfo& info) { callback__EndPointInfoUpdate(info); });
}

void IpcGraph::stop() {
  absl::MutexLock lock(&mutex_);
  discovery_.reset();
}

std::optional<heph::serdes::TypeInfo> IpcGraph::getTopicTypeInfo(const std::string& topic) const {
  absl::MutexLock lock(&mutex_);
  return topic_db_->getTypeInfo(topic);
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

TopicsToTypesMap IpcGraph::getTopicsToTypesMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.topics_to_types_map;
}

TopicsToTypesMap IpcGraph::getServicesToTypesMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.services_to_types_map;
}

TopicsToTypesMap IpcGraph::getServicesToNodesMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.services_to_nodes_map;
}

TopicToNodesMap IpcGraph::getTopicToSubscribersMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.topic_to_subscribers_map;
}

TopicToNodesMap IpcGraph::getTopicToPublishersMap() const {
  absl::MutexLock lock(&mutex_);
  return state_.topic_to_publishers_map;
}

void IpcGraph::addPublisher(const ipc::zenoh::EndpointInfo& info) {
  absl::MutexLock lock(&mutex_);
  if (!addTopic(info.topic)) {
    return;
  }

  state_.topic_to_publishers_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removePublisher(const ipc::zenoh::EndpointInfo& info) {
  absl::MutexLock lock(&mutex_);
  auto& publishers = state_.topic_to_publishers_map[info.topic];
  publishers.erase(std::remove(publishers.begin(), publishers.end(), info.session_id), publishers.end());
  if (publishers.empty()) {
    state_.topic_to_publishers_map.erase(info.topic);
    removeTopic(info.topic);
  }
}

bool IpcGraph::hasPublisher(const std::string& topic) {
  absl::MutexLock lock(&mutex_);
  return state_.topic_to_publishers_map.find(topic) != state_.topic_to_publishers_map.end();
}

void IpcGraph::addSubscriber(const ipc::zenoh::EndpointInfo& info) {
  absl::MutexLock lock(&mutex_);
  state_.topic_to_subscribers_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removeSubscriber(const ipc::zenoh::EndpointInfo& info) {
  absl::MutexLock lock(&mutex_);
  auto& subscribers = state_.topic_to_subscribers_map[info.topic];
  subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), info.session_id), subscribers.end());
  if (subscribers.empty()) {
    state_.topic_to_subscribers_map.erase(info.topic);
  }
}

void IpcGraph::removeTopic(const std::string& topic) {
  absl::MutexLock lock(&mutex_);
  state_.topics_to_types_map.erase(topic);
  state_.topic_to_publishers_map.erase(topic);
  state_.topic_to_subscribers_map.erase(topic);

  heph::log(heph::INFO, "[IPC Graph] - Topic dropped: '", topic, "'");

  if (config_.topic_removal_cb) {
    config_.topic_removal_cb(topic);
  }
}

bool IpcGraph::hasTopic(const std::string& topic_name) {
  absl::MutexLock lock(&mutex_);
  return state_.topics_to_types_map.find(topic_name) != state_.topics_to_types_map.end();
}

bool IpcGraph::addTopic(const std::string& topic) {
  absl::MutexLock lock(&mutex_);
  if (hasTopic(topic)) {
    heph::log(heph::ERROR, "[IPC Graph] - Trying to add a topic twice: '", topic, "'");
    return true;
  }

  auto type_info = topic_db_->getTypeInfo(topic);
  if (!type_info.has_value()) {
    heph::log(heph::ERROR, "[IPC Graph] - Could not retrieve type info for topic: '", topic, "'");
    return false;
  }

  state_.topics_to_types_map[topic] = type_info->name;

  heph::log(heph::INFO, "[IPC Graph] - Topic discovered: ", topic, " with type '", type_info->name, "'");

  if (config_.topic_discovery_cb) {
    config_.topic_discovery_cb(topic, type_info.value());
  }

  return true;
}

}  // namespace heph::ws_bridge
