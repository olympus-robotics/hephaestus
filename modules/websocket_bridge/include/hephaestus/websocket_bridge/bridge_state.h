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

#include "hephaestus/websocket_bridge/ws_server_utils.h"

namespace heph::ws_bridge {

class WsBridgeState {
public:
  WsBridgeState() = default;

  // Full State [protected by mutex_t2c_ & mutex_c2c_]
public:
  std::string toString() const;
  void printBridgeState() const;

  [[nodiscard]] bool checkConsistency() const;

  // Topics <-> Channels [protected by mutex_t2c_]
public:
  std::string getIpcTopicForWsChannel(const WsServerChannelId& channel_id) const;
  WsServerChannelId getWsChannelForIpcTopic(const std::string& topic) const;
  void addWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id, const std::string& topic);
  void removeWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id, const std::string& topic);
  bool hasWsChannelMapping(const WsServerChannelId& channel_id) const;
  bool hasIpcTopicMapping(const std::string& topic) const;
  std::string topicChannelMappingToString() const;

private:
  using ChannelToTopicMap = std::unordered_map<WsServerChannelId, std::string>;
  using TopicToChannelMap = std::unordered_map<std::string, WsServerChannelId>;

  mutable absl::Mutex mutex_t2c_;
  ChannelToTopicMap channel_to_topic_ ABSL_GUARDED_BY(mutex_t2c_);
  TopicToChannelMap topic_to_channel_ ABSL_GUARDED_BY(mutex_t2c_);

  // Channels <-> Clients [protected by mutex_c2c_]
public:
  bool hasWsChannelWithClients(const WsServerChannelId& channel_id) const;
  void addWsChannelToClientMapping(const WsServerChannelId& channel_id, WsServerClientHandle client_handle,
                                   const std::string& client_name);
  void removeWsChannelToClientMapping(const WsServerChannelId& channel_id);
  void removeWsChannelToClientMapping(const WsServerChannelId& channel_id,
                                      WsServerClientHandle client_handle);
  std::optional<WsServerClientHandleSet> getClientsForWsChannel(const WsServerChannelId& channel_id) const;

  std::string channelClientMappingToString() const;

private:
  void cleanUpChannelToClientMapping() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_c2c_);

  using ChannelToClientMap = std::unordered_map<WsServerChannelId, WsServerClientHandleSet>;
  mutable absl::Mutex mutex_c2c_;
  ChannelToClientMap channel_to_client_map_ ABSL_GUARDED_BY(mutex_c2c_);
};

}  // namespace heph::ws_bridge
