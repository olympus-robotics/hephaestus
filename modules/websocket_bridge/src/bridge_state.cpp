//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge_state.h"

namespace heph::ws_bridge {

std::string WsBridgeState::getIpcTopicForWsChannel(const WsServerChannelId& channel_id) const {
  absl::MutexLock lock(&mutex_t2c_);
  auto it = channel_to_topic_.find(channel_id);
  if (it == channel_to_topic_.end()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not convert channel id to topic. Something went wrong!",
              "channel id", std::to_string(channel_id));
    return "";
  }
  return it->second;
}

WsServerChannelId WsBridgeState::getWsChannelForIpcTopic(const std::string& topic) const {
  absl::MutexLock lock(&mutex_t2c_);
  auto it = topic_to_channel_.find(topic);
  if (it == topic_to_channel_.end()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not find channel id for topic. Something went wrong!",
              "topic", topic);
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
    return oss.str();
  }

  for (const auto& [channel_id, topic] : channel_to_topic_) {
    oss << "  \t'" << topic << "' -> [" << channel_id << "]\n";
  }
  return oss.str();
}

bool WsBridgeState::hasWsChannelWithClients(const WsServerChannelId& channel_id) const {
  absl::MutexLock lock(&mutex_c2c_);
  auto it = channel_to_client_map_.find(channel_id);
  const bool channel_is_in_map = it != channel_to_client_map_.end();
  const bool channel_has_clients = channel_is_in_map && !it->second.empty();
  if (channel_is_in_map && !channel_has_clients) {
    heph::log(heph::ERROR, "If a channel is in the map, it must have at least one client handle!",
              "channel id", std::to_string(channel_id));
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
      heph::log(heph::ERROR, "If a channel is in the map, it must have at least one client handle!",
                "channel id", std::to_string(channel_id));
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
    return oss.str();
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
     << channelClientMappingToString() << "\n"
     << servicMappingToString() << "\n"
     << callIdToClientMappingToString() << "\n"
     << "  CONSISTENCY CHECK: " << (checkConsistency() ? "PASS" : "FAIL") << "\n";

  return ss.str();
}

void WsBridgeState::printBridgeState() const {
  fmt::println("{}", toString());
}

bool WsBridgeState::checkConsistency() const {
  bool consistent = true;
  {
    absl::MutexLock lock(&mutex_t2c_);
    // Check channel -> topic consistency
    for (const auto& [channel, topic] : channel_to_topic_) {
      auto it = topic_to_channel_.find(topic);
      if (it == topic_to_channel_.end() || it->second != channel) {
        heph::log(heph::ERROR,
                  "[WS Bridge] Inconsistent state between channel_to_topic_ and topic_to_channel_.",
                  "channel", std::to_string(channel), "topic", topic);
        consistent = false;
      }
    }
    // Check topic -> channel consistency
    for (const auto& [topic, channel] : topic_to_channel_) {
      auto it = channel_to_topic_.find(channel);
      if (it == channel_to_topic_.end() || it->second != topic) {
        heph::log(heph::ERROR,
                  "[WS Bridge] Inconsistent state between topic_to_channel_ and channel_to_topic_.",
                  "channel", std::to_string(channel), "topic", topic);
        consistent = false;
      }
    }
  }

  {
    absl::MutexLock lock(&mutex_c2c_);
    // Check for channels with empty client sets
    for (const auto& [channel, clients] : channel_to_client_map_) {
      if (clients.empty()) {
        heph::log(heph::WARN, "[WS Bridge] A channel in channel_to_client_map_ has an empty client set.",
                  "channel", std::to_string(channel));
      }
    }
  }

  return consistent;
}

std::string WsBridgeState::getIpcServiceForWsService(const WsServerServiceId& service_id) const {
  absl::MutexLock lock(&mutex_s2s_);
  auto it = service_id_to_service_name_map_.find(service_id);
  if (it == service_id_to_service_name_map_.end()) {
    heph::log(heph::ERROR,
              "[WS Bridge] - Could not convert service id to service name. Something went wrong!",
              "service id", std::to_string(service_id));
    return "";
  }
  return it->second;
}

WsServerChannelId WsBridgeState::getWsServiceForIpcService(const std::string& service_name) const {
  absl::MutexLock lock(&mutex_s2s_);
  auto it = service_name_to_service_id_map_.find(service_name);
  if (it == service_name_to_service_id_map_.end()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not find service id for service name. Something went wrong!",
              "service name", service_name);
    return {};
  }
  return it->second;
}

void WsBridgeState::addWsServiceToIpcServiceMapping(const WsServerServiceId& service_id,
                                                    const std::string& service_name) {
  absl::MutexLock lock(&mutex_s2s_);
  service_id_to_service_name_map_[service_id] = service_name;
  service_name_to_service_id_map_[service_name] = service_id;
}

void WsBridgeState::removeWsServiceToIpcServiceMapping(const WsServerServiceId& service_id,
                                                       const std::string& service_name) {
  absl::MutexLock lock(&mutex_s2s_);
  service_id_to_service_name_map_.erase(service_id);
  service_name_to_service_id_map_.erase(service_name);
}

bool WsBridgeState::hasWsServiceMapping(const WsServerChannelId& service_id) const {
  absl::MutexLock lock(&mutex_s2s_);
  return service_id_to_service_name_map_.find(service_id) != service_id_to_service_name_map_.end();
}

bool WsBridgeState::hasIpcServiceMapping(const std::string& service_name) const {
  absl::MutexLock lock(&mutex_s2s_);
  return service_name_to_service_id_map_.find(service_name) != service_name_to_service_id_map_.end();
}

std::string WsBridgeState::servicMappingToString() const {
  absl::MutexLock lock(&mutex_s2s_);
  std::ostringstream oss;
  oss << "  IPC Service to WS Service Mapping:\n";

  if (service_id_to_service_name_map_.empty()) {
    oss << "  \t∅\n";
    return oss.str();
  }

  for (const auto& [service_id, service_name] : service_id_to_service_name_map_) {
    oss << "  \t'" << service_name << "' -> [" << service_id << "]\n";
  }
  return oss.str();
}

bool WsBridgeState::hasCallIdToClientMapping(const uint32_t& call_id) const {
  absl::MutexLock lock(&mutex_sc2c_);
  return call_id_to_client_map_.find(call_id) != call_id_to_client_map_.end();
}

void WsBridgeState::addCallIdToClientMapping(const uint32_t& call_id, WsServerClientHandle client_handle,
                                             const std::string& client_name) {
  absl::MutexLock lock(&mutex_sc2c_);
  call_id_to_client_map_[call_id] = { client_handle, client_name };

  if (client_handle.expired()) {
    heph::log(heph::WARN, "[App Bridge] Client hung up unexpectedly.");
  }

  cleanUpCallIdToClientMapping();
}

void WsBridgeState::removeCallIdToClientMapping(const uint32_t& call_id) {
  absl::MutexLock lock(&mutex_sc2c_);
  call_id_to_client_map_.erase(call_id);
}

std::optional<ClientHandleWithName> WsBridgeState::getClientForCallId(const uint32_t& call_id) const {
  absl::MutexLock lock(&mutex_sc2c_);
  auto it = call_id_to_client_map_.find(call_id);
  if (it == call_id_to_client_map_.end()) {
    return std::nullopt;
  }

  if (it->second.first.expired()) {
    heph::log(heph::ERROR, "If a call ID is in the map, it must have a valid client handle!", "call id",
              std::to_string(call_id));
  }

  return it->second;
}

std::string WsBridgeState::callIdToClientMappingToString() const {
  absl::MutexLock lock(&mutex_sc2c_);
  std::ostringstream oss;
  oss << "  WS Service Call ID to WS Client Mapping:\n";

  if (call_id_to_client_map_.empty()) {
    oss << "  \t∅\n";
    return oss.str();
  }

  for (const auto& [call_id, client_handle_w_name] : call_id_to_client_map_) {
    oss << "  \t[" << call_id << "] -> '" << client_handle_w_name.second << "' ("
        << (client_handle_w_name.first.expired() ? "expired" : "valid") << ")\n";
  }
  return oss.str();
}

void WsBridgeState::cleanUpCallIdToClientMapping() {
  // Remove all expired client handles from the call ID to client map
  for (auto it = call_id_to_client_map_.begin(); it != call_id_to_client_map_.end();) {
    if (it->second.first.expired()) {
      it = call_id_to_client_map_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace heph::ws_bridge
