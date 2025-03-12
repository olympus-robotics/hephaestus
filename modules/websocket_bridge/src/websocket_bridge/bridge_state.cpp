//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge_state.h"

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include <absl/synchronization/mutex.h>
#include <fmt/base.h>
#include <hephaestus/telemetry/log.h>

#include "hephaestus/websocket_bridge/utils/ws_protocol.h"

namespace heph::ws {

auto WebsocketBridgeState::getIpcTopicForWsChannel(const WsChannelId& channel_id) const -> std::string {
  const absl::ReaderMutexLock lock(&mutex_t2c_);
  auto it = channel_to_topic_.find(channel_id);
  if (it == channel_to_topic_.end()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not convert channel id to topic. Something went wrong!",
              "channel id", std::to_string(channel_id));
    return "";
  }
  return it->second;
}

auto WebsocketBridgeState::getWsChannelForIpcTopic(const std::string& topic) const -> WsChannelId {
  const absl::ReaderMutexLock lock(&mutex_t2c_);
  auto it = topic_to_channel_.find(topic);
  if (it == topic_to_channel_.end()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not find channel id for topic. Something went wrong!",
              "topic", topic);
    return {};
  }
  return it->second;
}

void WebsocketBridgeState::addWsChannelToIpcTopicMapping(const WsChannelId& channel_id,
                                                         const std::string& topic) {
  const absl::WriterMutexLock lock(&mutex_t2c_);
  channel_to_topic_[channel_id] = topic;
  topic_to_channel_[topic] = channel_id;
}

void WebsocketBridgeState::removeWsChannelToIpcTopicMapping(const WsChannelId& channel_id,
                                                            const std::string& topic) {
  const absl::WriterMutexLock lock(&mutex_t2c_);
  channel_to_topic_.erase(channel_id);
  topic_to_channel_.erase(topic);
}

auto WebsocketBridgeState::hasWsChannelMapping(const WsChannelId& channel_id) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_t2c_);
  return channel_to_topic_.find(channel_id) != channel_to_topic_.end();
}

auto WebsocketBridgeState::hasIpcTopicMapping(const std::string& topic) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_t2c_);
  return topic_to_channel_.find(topic) != topic_to_channel_.end();
}

auto WebsocketBridgeState::hasWsChannelWithClients(const WsChannelId& channel_id) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_c2c_);
  auto it = channel_to_client_map_.find(channel_id);
  const bool channel_is_in_map = it != channel_to_client_map_.end();
  const bool channel_has_clients = channel_is_in_map && !it->second.empty();
  if (channel_is_in_map && !channel_has_clients) {
    heph::log(heph::ERROR, "If a channel is in the map, it must have at least one client handle!",
              "channel id", std::to_string(channel_id));
  }
  return channel_has_clients;
}

void WebsocketBridgeState::addWsChannelToClientMapping(const WsChannelId& channel_id,
                                                       const WsClientHandle& client_handle,
                                                       const std::string& client_name) {
  const absl::WriterMutexLock lock(&mutex_c2c_);
  channel_to_client_map_[channel_id].emplace(client_handle, client_name);

  if (client_handle.expired()) {
    heph::log(heph::WARN, "[App Bridge] Client hung up unexpectedly.");
  }

  cleanUpChannelToClientMapping();
}

void WebsocketBridgeState::removeWsChannelToClientMapping(const WsChannelId& channel_id) {
  const absl::WriterMutexLock lock(&mutex_c2c_);
  channel_to_client_map_.erase(channel_id);
}

void WebsocketBridgeState::removeWsChannelToClientMapping(const WsChannelId& channel_id,
                                                          const WsClientHandle& client_handle) {
  const absl::WriterMutexLock lock(&mutex_c2c_);
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

auto WebsocketBridgeState::getClientsForWsChannel(const WsChannelId& channel_id) const
    -> std::optional<WsClientHandleSet> {
  const absl::ReaderMutexLock lock(&mutex_c2c_);
  auto it = channel_to_client_map_.find(channel_id);
  if (it == channel_to_client_map_.end()) {
    return std::nullopt;
  }

  WsClientHandleSet client_handles;
  for (const auto& client : it->second) {
    if (client.first.expired()) {
      heph::log(heph::ERROR, "If a channel is in the map, it must have at least one client handle!",
                "channel id", std::to_string(channel_id));
    }
    client_handles.insert(client);
  }

  return std::make_optional(client_handles);
}

void WebsocketBridgeState::cleanUpChannelToClientMapping() {
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

void WebsocketBridgeState::printBridgeState() const {
  fmt::println("{}", toString());
}

auto WebsocketBridgeState::checkConsistency() const -> bool {
  bool consistent = true;
  {
    const absl::ReaderMutexLock lock(&mutex_t2c_);
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
    const absl::ReaderMutexLock lock(&mutex_c2c_);
    // Check for channels with empty client sets
    for (const auto& [channel, clients] : channel_to_client_map_) {
      if (clients.empty()) {
        heph::log(heph::WARN, "[WS Bridge] A channel in channel_to_client_map_ has an empty client set.",
                  "channel", std::to_string(channel));
      }
    }
  }

  {
    const absl::ReaderMutexLock lock(&mutex_s2s_);
    // Check service id -> service name consistency
    for (const auto& [service_id, service_name] : service_id_to_service_name_map_) {
      auto it = service_name_to_service_id_map_.find(service_name);
      if (it == service_name_to_service_id_map_.end() || it->second != service_id) {
        heph::log(heph::ERROR,
                  "[WS Bridge] Inconsistent state between service_id_to_service_name_map_ and "
                  "service_name_to_service_id_map_.",
                  "service_id", std::to_string(service_id), "service_name", service_name);
        consistent = false;
      }
    }
    // Check service name -> service id consistency
    for (const auto& [service_name, service_id] : service_name_to_service_id_map_) {
      auto it = service_id_to_service_name_map_.find(service_id);
      if (it == service_id_to_service_name_map_.end() || it->second != service_name) {
        heph::log(heph::ERROR,
                  "[WS Bridge] Inconsistent state between service_name_to_service_id_map_ and "
                  "service_id_to_service_name_map_.",
                  "service_id", std::to_string(service_id), "service_name", service_name);
        consistent = false;
      }
    }
  }

  {
    // Check client_channel -> ipc_topic consistency
    const absl::ReaderMutexLock lock(&mutex_cc2t_);
    for (const auto& [client_channel_id, topic] : client_channel_to_topic_) {
      auto it = topic_to_client_channels_.find(topic);
      if (it == topic_to_client_channels_.end() || it->second.find(client_channel_id) == it->second.end()) {
        heph::log(heph::ERROR,
                  "[WS Bridge] Inconsistent state between client_channel_to_topic_ and "
                  "topic_to_client_channels_.",
                  "client_channel_id", client_channel_id, "topic", topic);
        consistent = false;
      }
    }
    // Check ipc_topic -> client_channel consistency
    for (const auto& [topic, client_channel_ids] : topic_to_client_channels_) {
      for (const auto& client_channel_id : client_channel_ids) {
        auto it = client_channel_to_topic_.find(client_channel_id);
        if (it == client_channel_to_topic_.end() || it->second != topic) {
          heph::log(heph::ERROR,
                    "[WS Bridge] Inconsistent state between topic_to_client_channels_ and "
                    "client_channel_to_topic_.",
                    "client_channel_id", client_channel_id, "topic", topic);
          consistent = false;
        }
      }
    }
  }

  return consistent;
}

auto WebsocketBridgeState::getIpcServiceForWsService(const WsServiceId& service_id) const -> std::string {
  const absl::ReaderMutexLock lock(&mutex_s2s_);
  auto it = service_id_to_service_name_map_.find(service_id);
  if (it == service_id_to_service_name_map_.end()) {
    heph::log(heph::ERROR,
              "[WS Bridge] - Could not convert service id to service name. Something went wrong!",
              "service id", std::to_string(service_id));
    return "";
  }
  return it->second;
}

auto WebsocketBridgeState::getWsServiceForIpcService(const std::string& service_name) const -> WsChannelId {
  const absl::ReaderMutexLock lock(&mutex_s2s_);
  auto it = service_name_to_service_id_map_.find(service_name);
  if (it == service_name_to_service_id_map_.end()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not find service id for service name. Something went wrong!",
              "service name", service_name);
    return {};
  }
  return it->second;
}

void WebsocketBridgeState::addWsServiceToIpcServiceMapping(const WsServiceId& service_id,
                                                           const std::string& service_name) {
  const absl::WriterMutexLock lock(&mutex_s2s_);
  service_id_to_service_name_map_[service_id] = service_name;
  service_name_to_service_id_map_[service_name] = service_id;
}

void WebsocketBridgeState::removeWsServiceToIpcServiceMapping(const WsServiceId& service_id,
                                                              const std::string& service_name) {
  const absl::WriterMutexLock lock(&mutex_s2s_);
  service_id_to_service_name_map_.erase(service_id);
  service_name_to_service_id_map_.erase(service_name);
}

auto WebsocketBridgeState::hasWsServiceMapping(const WsChannelId& service_id) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_s2s_);
  return service_id_to_service_name_map_.find(service_id) != service_id_to_service_name_map_.end();
}

auto WebsocketBridgeState::hasIpcServiceMapping(const std::string& service_name) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_s2s_);
  return service_name_to_service_id_map_.find(service_name) != service_name_to_service_id_map_.end();
}

auto WebsocketBridgeState::hasCallIdToClientMapping(const uint32_t& call_id) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_sc2c_);
  return call_id_to_client_map_.find(call_id) != call_id_to_client_map_.end();
}

void WebsocketBridgeState::addCallIdToClientMapping(const uint32_t& call_id,
                                                    const WsClientHandle& client_handle,
                                                    const std::string& client_name) {
  const absl::WriterMutexLock lock(&mutex_sc2c_);
  call_id_to_client_map_[call_id] = { client_handle, client_name };

  if (client_handle.expired()) {
    heph::log(heph::WARN, "[App Bridge] Client hung up unexpectedly.");
  }

  cleanUpCallIdToClientMapping();
}

void WebsocketBridgeState::removeCallIdToClientMapping(const uint32_t& call_id) {
  const absl::WriterMutexLock lock(&mutex_sc2c_);
  call_id_to_client_map_.erase(call_id);
}

auto WebsocketBridgeState::getClientForCallId(const uint32_t& call_id) const
    -> std::optional<ClientHandleWithName> {
  const absl::ReaderMutexLock lock(&mutex_sc2c_);
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

void WebsocketBridgeState::cleanUpCallIdToClientMapping() {
  // Remove all expired client handles from the call ID to client map
  for (auto it = call_id_to_client_map_.begin(); it != call_id_to_client_map_.end();) {
    if (it->second.first.expired()) {
      it = call_id_to_client_map_.erase(it);
    } else {
      ++it;
    }
  }
}

auto WebsocketBridgeState::hasClientChannelsForTopic(const std::string& topic) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_cc2t_);
  return topic_to_client_channels_.find(topic) != topic_to_client_channels_.end();
}

auto WebsocketBridgeState::getTopicForClientChannel(const WsClientChannelId& client_channel_id) const
    -> std::string {
  const absl::ReaderMutexLock lock(&mutex_cc2t_);
  auto it = client_channel_to_topic_.find(client_channel_id);
  if (it == client_channel_to_topic_.end()) {
    heph::log(heph::ERROR,
              "[WS Bridge] - Could not convert client channel id to topic. Something went wrong!",
              "client channel id", std::to_string(client_channel_id));
    return "";
  }
  return it->second;
}

auto WebsocketBridgeState::getClientChannelsForTopic(const std::string& topic) const -> WsClientChannelIdSet {
  const absl::ReaderMutexLock lock(&mutex_cc2t_);
  auto it = topic_to_client_channels_.find(topic);
  if (it == topic_to_client_channels_.end()) {
    heph::log(heph::ERROR,
              "[WS Bridge] - Could not find any client channel ids for topic. Something went wrong!", "topic",
              topic);
    return {};
  }
  return it->second;
}

void WebsocketBridgeState::addClientChannelToTopicMapping(const WsClientChannelId& client_channel_id,
                                                          const std::string& topic) {
  const absl::WriterMutexLock lock(&mutex_cc2t_);

  auto it = client_channel_to_topic_.find(client_channel_id);
  if (it != client_channel_to_topic_.end()) {
    const std::string existing_topic = it->second;
    if (existing_topic == topic) {
      heph::log(heph::WARN, "[WS Bridge] - Client channel already mapped to topic!", "client_channel_id",
                std::to_string(client_channel_id), "topic", topic);
      return;
    }

    heph::log(heph::ERROR, "[WS Bridge] - Client channel already mapped to a different topic!",
              "client_channel_id", std::to_string(client_channel_id), "existing_topic", existing_topic,
              "new_topic", topic);
    return;
  }

  client_channel_to_topic_[client_channel_id] = topic;
  topic_to_client_channels_[topic].insert(client_channel_id);
}

void WebsocketBridgeState::removeClientChannelToTopicMapping(const WsClientChannelId& client_channel_id) {
  const absl::WriterMutexLock lock(&mutex_cc2t_);
  auto it = client_channel_to_topic_.find(client_channel_id);
  if (it != client_channel_to_topic_.end()) {
    const std::string topic = it->second;
    client_channel_to_topic_.erase(it);

    // Remove client channel ID from the set
    auto topic_it = topic_to_client_channels_.find(topic);
    if (topic_it != topic_to_client_channels_.end()) {
      topic_it->second.erase(client_channel_id);

      // If the set is now empty, remove the topic entry
      if (topic_it->second.empty()) {
        topic_to_client_channels_.erase(topic_it);
      }
    }
  }
}

auto WebsocketBridgeState::hasClientChannelMapping(const WsClientChannelId& client_channel_id) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_cc2t_);
  return client_channel_to_topic_.find(client_channel_id) != client_channel_to_topic_.end();
}

auto WebsocketBridgeState::hasTopicToClientChannelMapping(const std::string& topic) const -> bool {
  const absl::ReaderMutexLock lock(&mutex_cc2t_);
  auto it = topic_to_client_channels_.find(topic);
  return it != topic_to_client_channels_.end() && !it->second.empty();
}

auto WebsocketBridgeState::hasClientForClientChannel(const WsClientChannelId& client_channel_id) const
    -> bool {
  const absl::ReaderMutexLock lock(&mutex_cc2c_);
  auto it = client_channel_to_client_map_.find(client_channel_id);
  const bool channel_is_in_map = it != client_channel_to_client_map_.end();

  if (channel_is_in_map) {
    if (it->second.first.expired()) {
      return false;
    }
    return true;
  }
  return false;
}

void WebsocketBridgeState::addClientChannelToClientMapping(const WsClientChannelId& client_channel_id,
                                                           const WsClientHandle& client_handle,
                                                           const std::string& client_name) {
  const absl::WriterMutexLock lock(&mutex_cc2c_);
  client_channel_to_client_map_[client_channel_id] = { client_handle, client_name };

  cleanUpClientChannelToClientMapping();
}

void WebsocketBridgeState::removeClientChannelToClientMapping(const WsClientChannelId& client_channel_id) {
  const absl::WriterMutexLock lock(&mutex_cc2c_);
  client_channel_to_client_map_.erase(client_channel_id);

  cleanUpClientChannelToClientMapping();
}

auto WebsocketBridgeState::getClientForClientChannel(const WsClientChannelId& client_channel_id) const
    -> std::optional<ClientHandleWithName> {
  const absl::ReaderMutexLock lock(&mutex_cc2c_);
  auto it = client_channel_to_client_map_.find(client_channel_id);
  if (it == client_channel_to_client_map_.end()) {
    return std::nullopt;
  }

  if (it->second.first.expired()) {
    heph::log(heph::ERROR, "If a client channel ID is in the map, it must have a valid client handle!",
              "client_channel_id", std::to_string(client_channel_id));
    return std::nullopt;
  }

  return std::make_optional(it->second);
}

void WebsocketBridgeState::cleanUpClientChannelToClientMapping() {
  // Remove all dead client handles from the client channel to client map
  for (auto channel_it = client_channel_to_client_map_.begin();
       channel_it != client_channel_to_client_map_.end();) {
    auto& client_handle_w_name = channel_it->second;

    if (client_handle_w_name.first.expired()) {
      channel_it = client_channel_to_client_map_.erase(channel_it);
    } else {
      ++channel_it;
    }
  }
}

/*
ADVERTISED TOPICS
'pub/test1' ===> [2]

  '127.0.0.1:54244' (valid)
IPC Service to WS Service Mapping:
'topic_info/pub/test1' -> [2]

CLIENT ADVERTISED TOPICS
'mirror/pub/test1'
  [10002] <=== <unknown client>


CONSISTENCY CHECK: PASS
*/

auto WebsocketBridgeState::toString() const -> std::string {
  std::stringstream ss;
  ss << "[WS Bridge] - State:\n"
     << "\n"
     << topicChannelMappingToString() << "\n"
     << servicMappingToString() << "\n"
     << clientChannelMappingToString() << "\n"
     << callIdToClientMappingToString() << "\n"
     << "  CONSISTENCY CHECK: " << (checkConsistency() ? "PASS" : "FAIL") << "\n";

  return ss.str();
}

auto WebsocketBridgeState::topicChannelMappingToString() const -> std::string {
  const absl::ReaderMutexLock lock(&mutex_t2c_);
  const absl::ReaderMutexLock second_lock(&mutex_c2c_);

  std::ostringstream oss;
  oss << "  ADVERTISED TOPICS\n";

  if (channel_to_topic_.empty()) {
    oss << "    ∅\n";
    return oss.str();
  }

  for (const auto& [channel_id, topic] : channel_to_topic_) {
    oss << "    '" << topic << "' ===> [" << channel_id << "]\n";

    auto it = channel_to_client_map_.find(channel_id);
    if (it != channel_to_client_map_.end()) {
      for (const auto& client : it->second) {
        oss << "      '" << client.second << "' (" << (client.first.expired() ? "expired" : "valid") << ")\n";
      }
    } else {
      oss << "      ∅\n";
    }
  }
  return oss.str();
}

auto WebsocketBridgeState::servicMappingToString() const -> std::string {
  const absl::ReaderMutexLock lock(&mutex_s2s_);
  std::ostringstream oss;
  oss << "  ADVERTISED SERVICES\n";

  if (service_id_to_service_name_map_.empty()) {
    oss << "    ∅\n";
    return oss.str();
  }

  for (const auto& [service_id, service_name] : service_id_to_service_name_map_) {
    oss << "    '" << service_name << "' <==> [" << service_id << "]\n";
  }
  return oss.str();
}

auto WebsocketBridgeState::callIdToClientMappingToString() const -> std::string {
  const absl::ReaderMutexLock lock(&mutex_sc2c_);
  std::ostringstream oss;
  oss << "  ACTIVE SERVICE CALLS\n";

  if (call_id_to_client_map_.empty()) {
    oss << "    ∅\n";
    return oss.str();
  }

  for (const auto& [call_id, client_handle_w_name] : call_id_to_client_map_) {
    oss << "    [" << call_id << "] <==> '" << client_handle_w_name.second << "' ("
        << (client_handle_w_name.first.expired() ? "expired" : "valid") << ")\n";
  }
  return oss.str();
}

auto WebsocketBridgeState::clientChannelMappingToString() const -> std::string {
  const absl::ReaderMutexLock lock(&mutex_cc2t_);
  const absl::ReaderMutexLock second_lock(&mutex_cc2c_);

  std::ostringstream oss;
  oss << "  CLIENT ADVERTISED TOPICS\n";

  if (topic_to_client_channels_.empty()) {
    oss << "    ∅\n";
    return oss.str();
  }

  for (const auto& [topic, client_channels] : topic_to_client_channels_) {
    oss << "    '" << topic << "'\n";
    for (const auto& client_channel_id : client_channels) {
      oss << "      [" << client_channel_id << "] <=== ";
      auto it = client_channel_to_client_map_.find(client_channel_id);
      if (it != client_channel_to_client_map_.end()) {
        const auto& client = it->second;
        oss << "'" << client.second << "' (" << (client.first.expired() ? "expired" : "valid") << ")\n";
      } else {
        oss << "<unknown client>\n";
      }
    }
  }
  return oss.str();
}

}  // namespace heph::ws
