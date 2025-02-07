//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ws_bridge/ws_bridge_state.h"

namespace heph::ws_bridge {

std::string WsBridgeState::getIpcTopicForWsChannel(const WsServerChannelId& channel_id) const {
  absl::MutexLock lock(&mutex_t2c_);
  auto it = channel_to_topic_.find(channel_id);
  if (it == channel_to_topic_.end()) {
    heph::log(heph::ERROR, toString());
    heph::log(heph::ERROR, "[WS Bridge] - Could not convert channel id [", std::to_string(channel_id),
              "] to topic. Something went wrong!");
    return "";
  }
  return it->second;
}

WsServerChannelId WsBridgeState::getWsChannelForIpcTopic(const std::string& topic) const {
  absl::MutexLock lock(&mutex_t2c_);
  auto it = topic_to_channel_.find(topic);
  if (it == topic_to_channel_.end()) {
    heph::log(heph::ERROR, toString());
    heph::log(heph::ERROR, "[WS Bridge] - Could not find channel id for topic '", topic,
              "'. Something went wrong!");
    return {};
  }
  return it->second;
}

void WsBridgeState::addWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id,
                                                  const std::string& topic) {
  absl::MutexLock lock(&mutex_t2c_);
  channel_to_topic_[channel_id] = topic;
  topic_to_channel_[topic] = channel_id;
}

void WsBridgeState::removeWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id,
                                                     const std::string& topic) {
  absl::MutexLock lock(&mutex_t2c_);
  channel_to_topic_.erase(channel_id);
  topic_to_channel_.erase(topic);
}

bool WsBridgeState::hasWsChannelMapping(const WsServerChannelId& channel_id) const {
  absl::MutexLock lock(&mutex_t2c_);
  return channel_to_topic_.find(channel_id) != channel_to_topic_.end();
}

bool WsBridgeState::hasIpcTopicMapping(const std::string& topic) const {
  absl::MutexLock lock(&mutex_t2c_);
  return topic_to_channel_.find(topic) != topic_to_channel_.end();
}

std::string WsBridgeState::topicChannelMappingToString() const {
  absl::MutexLock lock(&mutex_t2c_);
  std::ostringstream oss;
  oss << "  IPC Topic to WS Channel Mapping:\n";

  if (channel_to_topic_.empty()) {
    oss << "  \t∅\n";
  }

  for (const auto& [channel_id, topic] : channel_to_topic_) {
    oss << "  \t[" << channel_id << "] -> '" << topic << "'\n";
  }
  return oss.str();
}

bool WsBridgeState::hasWsChannelWithClients(const WsServerChannelId& channel_id) const {
  absl::MutexLock lock(&mutex_c2c_);
  auto it = channel_to_client_map_.find(channel_id);
  const bool channel_is_in_map = it != channel_to_client_map_.end();
  const bool channel_has_clients = channel_is_in_map && !it->second.empty();
  if (channel_is_in_map && !channel_has_clients) {
    heph::log(heph::ERROR, "If a channel [", std::to_string(channel_id),
              "] is in the map, it must have at least one client handle!");
  }
  return channel_has_clients;
}

void WsBridgeState::addWsChannelToClientMapping(const WsServerChannelId& channel_id,
                                                WsServerClientHandle client_handle,
                                                const std::string& client_name) {
  absl::MutexLock lock(&mutex_c2c_);
  channel_to_client_map_[channel_id].emplace(client_handle, client_name);

  if (client_handle.expired()) {
    heph::log(heph::WARN, "[App Bridge] Client hung up unexpectedly.");
  }

  cleanUpChannelToClientMapping();
}

void WsBridgeState::removeWsChannelToClientMapping(const WsServerChannelId& channel_id) {
  absl::MutexLock lock(&mutex_c2c_);
  channel_to_client_map_.erase(channel_id);
}

void WsBridgeState::removeWsChannelToClientMapping(const WsServerChannelId& channel_id,
                                                   WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_c2c_);
  auto it = channel_to_client_map_.find(channel_id);
  if (it != channel_to_client_map_.end()) {
    auto& clients = it->second;
    clients.erase(ClientHandleWithName(client_handle, ""));
    if (clients.empty()) {
      channel_to_client_map_.erase(it);
    }
  }

  if (client_handle.expired()) {
    heph::log(heph::WARN, "[App Bridge] Client hung up unexpectedly.");
  }

  cleanUpChannelToClientMapping();
}

std::optional<WsServerClientHandleSet>
WsBridgeState::getClientsForWsChannel(const WsServerChannelId& channel_id) const {
  absl::MutexLock lock(&mutex_c2c_);
  auto it = channel_to_client_map_.find(channel_id);
  if (it == channel_to_client_map_.end()) {
    return std::nullopt;
  }

  WsServerClientHandleSet client_handles;
  for (const auto& client : it->second) {
    if (client.first.expired()) {
      heph::log(heph::ERROR, "If a channel [", std::to_string(channel_id),
                "] is in the map, it must have at least one client handle!");
    }
    client_handles.insert(client);
  }

  return std::make_optional(client_handles);
}

std::string WsBridgeState::channelClientMappingToString() const {
  absl::MutexLock lock(&mutex_c2c_);
  std::ostringstream oss;
  oss << "  WS Channel to WS Client Mapping:\n";

  if (channel_to_client_map_.empty()) {
    oss << "  \t∅\n";
  }

  for (const auto& [channel_id, clients] : channel_to_client_map_) {
    oss << "  \t[" << channel_id << "]\n";
    for (const auto& client : clients) {
      oss << "  \t  - '" << client.second << "' (" << (client.first.expired() ? "expired" : "valid") << ")\n";
    }
  }
  return oss.str();
}

void WsBridgeState::cleanUpChannelToClientMapping() {
  // Remove all dead client handles from the channel to client map
  for (auto channel_it = channel_to_client_map_.begin(); channel_it != channel_to_client_map_.end();) {
    auto& clients = channel_it->second;
    for (auto it = clients.begin(); it != clients.end();) {
      if (it->first.expired()) {
        it = clients.erase(it);
      } else {
        ++it;
      }
    }

    if (clients.empty()) {
      channel_it = channel_to_client_map_.erase(channel_it);
    } else {
      ++channel_it;
    }
  }
}

std::string WsBridgeState::toString() const {
  std::stringstream ss;
  ss << "[WS Bridge] - State:\n"
     << "\n"
     << topicChannelMappingToString() << "\n"
     << channelClientMappingToString();
  return ss.str();
}

}  // namespace heph::ws_bridge
