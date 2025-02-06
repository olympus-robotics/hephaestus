#include "hephaestus/ws_bridge/ipc_graph.h"

#include <memory>
#include <string>
#include <vector>

#include <absl/log/check.h>
#include <hephaestus/ipc/zenoh/liveliness.h>
#include <hephaestus/telemetry/log.h>

namespace heph::ws_bridge {

IpcGraph::IpcGraph(const IpcGraphConfig& config)
  : session_(std::move(config.session))
  , topic_db_(ipc::createZenohTopicDatabase(session_))
  , topic_removal_cb_(config.topic_removal_cb)
  , topic_discovery_cb_(config.topic_discovery_cb) {
}

void IpcGraph::CallbackLivelinessUpdate(const ipc::zenoh::EndpointInfo& info) {
  switch (info.type) {
    case ipc::zenoh::EndpointType::SERVICE_SERVER:
    case ipc::zenoh::EndpointType::SERVICE_CLIENT:
    case ipc::zenoh::EndpointType::ACTION_SERVER:
      break;
    case ipc::zenoh::EndpointType::PUBLISHER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addPublisher(info);
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removePublisher(info);
      }
      break;
    case ipc::zenoh::EndpointType::SUBSCRIBER:
      switch (info.status) {
        case ipc::zenoh::EndpointInfo::Status::ALIVE:
          addSubscriber(info);
          break;
        case ipc::zenoh::EndpointInfo::Status::DROPPED:
          removeSubscriber(info);
      }
      break;
  }
}

void IpcGraph::Start() {
  discovery_ = std::make_unique<ipc::zenoh::EndpointDiscovery>(
      session_, ipc::TopicConfig{ "**" },
      [this](const ipc::zenoh::EndpointInfo& info) { CallbackLivelinessUpdate(info); });
}

void IpcGraph::Stop() {
  discovery_.reset();
}

std::optional<heph::serdes::TypeInfo> IpcGraph::getTopicTypeInfo(const std::string& topic) const {
  return topic_db_->getTypeInfo(topic);
}

std::string IpcGraph::GetTopicListString() {
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

TopicsToTypesMap IpcGraph::GetTopicsToTypesMap() const {
  return state_.topics_to_types_map;
}

TopicsToTypesMap IpcGraph::GetServicesToTypesMap() const {
  return state_.services_to_types_map;
}

TopicsToTypesMap IpcGraph::GetServicesToNodesMap() const {
  return state_.services_to_nodes_map;
}

TopicToNodesMap IpcGraph::GetTopicToSubscribersMap() const {
  return state_.topic_to_subscribers_map;
}

TopicToNodesMap IpcGraph::GetTopicToPublishersMap() const {
  return state_.topic_to_publishers_map;
}

void IpcGraph::addPublisher(const ipc::zenoh::EndpointInfo& info) {
  if (!addTopic(info.topic)) {
    return;
  }

  state_.topic_to_publishers_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removePublisher(const ipc::zenoh::EndpointInfo& info) {
  auto& publishers = state_.topic_to_publishers_map[info.topic];
  publishers.erase(std::remove(publishers.begin(), publishers.end(), info.session_id), publishers.end());
  if (publishers.empty()) {
    state_.topic_to_publishers_map.erase(info.topic);
    removeTopic(info.topic);
  }
}

bool IpcGraph::hasPublisher(const std::string& topic) {
  return state_.topic_to_publishers_map.find(topic) != state_.topic_to_publishers_map.end();
}

void IpcGraph::addSubscriber(const ipc::zenoh::EndpointInfo& info) {
  state_.topic_to_subscribers_map[info.topic].push_back(info.session_id);
}

void IpcGraph::removeSubscriber(const ipc::zenoh::EndpointInfo& info) {
  auto& subscribers = state_.topic_to_subscribers_map[info.topic];
  subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), info.session_id), subscribers.end());
  if (subscribers.empty()) {
    state_.topic_to_subscribers_map.erase(info.topic);
  }
}

void IpcGraph::removeTopic(const std::string& topic) {
  state_.topics_to_types_map.erase(topic);
  state_.topic_to_publishers_map.erase(topic);
  state_.topic_to_subscribers_map.erase(topic);

  heph::log(heph::INFO, "[IPC Graph] - Topic dropped: '", topic, "'");
  topic_removal_cb_(topic);
}

bool IpcGraph::hasTopic(const std::string& topic_name) {
  return state_.topics_to_types_map.find(topic_name) != state_.topics_to_types_map.end();
}

bool IpcGraph::addTopic(const std::string& topic) {
  if (hasTopic(topic)) {
    heph::log(heph::ERROR, "[IPC Graph] - Trying to add a topic twice: '", topic, "'");
    return true;
  }

  auto type_info = getTopicTypeInfo(topic);
  if (!type_info.has_value()) {
    heph::log(heph::ERROR, "[IPC Graph] - Could not retrieve type info for topic: '", topic, "'");
    return false;
  }

  state_.topics_to_types_map[topic] = type_info->name;

  heph::log(heph::INFO, "[IPC Graph] - Topic discovered: ", topic, " with type '", type_info->name, "'");
  topic_discovery_cb_(topic, type_info.value());
  return true;
}

}  // namespace heph::ws_bridge
