//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

#include <absl/synchronization/mutex.h>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/telemetry/log.h>

#include "hephaestus/ws_bridge/ws_server_utils.h"

namespace heph::ws_bridge {

class WsBridgeState {
public:
  WsBridgeState() = default;

private:
  mutable absl::Mutex mutex_;

  // Topics <-> Channels
public:
  std::string getIpcTopicForWsChannel(const WsServerChannelId& channel_id) const;
  WsServerChannelId getWsChannelForIpcTopic(const std::string& topic) const;
  void addWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id, const std::string& topic);
  void removeWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id, const std::string& topic);
  bool hasWsChannelMapping(const WsServerChannelId& channel_id) const;
  bool hasIpcTopicMapping(const std::string& topic) const;

private:
  std::string topicChannelMappingToString() const;
  using ChannelToTopicMap = std::unordered_map<WsServerChannelId, std::string>;
  using TopicToChannelMap = std::unordered_map<std::string, WsServerChannelId>;
  ChannelToTopicMap channel_to_topic_;
  TopicToChannelMap topic_to_channel_;

  // Channels <-> Clients
public:
  bool hasWsChannelWithClients(const WsServerChannelId& channel_id) const;
  void addWsChannelToClientMapping(const WsServerChannelId& channel_id, WsServerClientHandle client_handle,
                                   const std::string& client_name);
  void removeWsChannelToClientMapping(const WsServerChannelId& channel_id);
  void removeWsChannelToClientMapping(const WsServerChannelId& channel_id,
                                      WsServerClientHandle client_handle);
  std::optional<WsServerClientHandleSet> getClientsForWsChannel(const WsServerChannelId& channel_id) const;
  std::string toString() const;

private:
  std::string channelClientMappingToString() const;
  void cleanUpChannelToClientMapping();

  using ChannelToClientMap = std::unordered_map<WsServerChannelId, WsServerClientHandleSet>;
  ChannelToClientMap channel_to_client_map_;
};

}  // namespace heph::ws_bridge
