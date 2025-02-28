//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

#include <absl/synchronization/mutex.h>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/telemetry/log.h>

#include "hephaestus/utils/ws_protocol.h"

namespace heph::ws_bridge {

class WsBridgeState {
public:
  WsBridgeState() = default;

  // Full State [protected by all mutexes]
public:
  std::string toString() const;
  void printBridgeState() const;

  [[nodiscard]] bool checkConsistency() const;

  // IPC Topics <-> WS Channels [protected by mutex_t2c_]
  // Keeps track of which IPC topic maps to which WS channel.
  // Assumptions:
  // - One IPC topic, one WS channel and vice versa
  // - Topic names and channel IDs are unique
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

  // WS Channels <-> WS Clients [protected by mutex_c2c_]
  // Keeps track of which channel was requested by which client, hence which client needs to receive incoming
  // messages.
  // Assumptions:
  // - One channel, many clients
  // - Topic names and channel IDs are unique
  // - Client can one-sided hang up asynchronously and invalidate their handle, hence lookups can fail.
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

  // IPC Services <-> WS Service [protected by mutex_s2s_]
  // Keeps track of which IPC service is mapped to which WS service.
  // Assumptions:
  // - One IPC service, one WS service and vice versa
  // - Service names and service IDs are unique
public:
  std::string getIpcServiceForWsService(const WsServerServiceId& service_id) const;
  WsServerChannelId getWsServiceForIpcService(const std::string& service_name) const;
  void addWsServiceToIpcServiceMapping(const WsServerServiceId& service_id, const std::string& service_name);
  void removeWsServiceToIpcServiceMapping(const WsServerServiceId& service_id,
                                          const std::string& service_name);
  bool hasWsServiceMapping(const WsServerChannelId& service_id) const;
  bool hasIpcServiceMapping(const std::string& service_name) const;
  std::string servicMappingToString() const;

private:
  using ServiceNameToServiceIdMap = std::unordered_map<std::string, WsServerServiceId>;
  using ServiceIdToServiceNameMap = std::unordered_map<WsServerServiceId, std::string>;

  mutable absl::Mutex mutex_s2s_;

  ServiceNameToServiceIdMap service_name_to_service_id_map_ ABSL_GUARDED_BY(mutex_s2s_);
  ServiceIdToServiceNameMap service_id_to_service_name_map_ ABSL_GUARDED_BY(mutex_s2s_);

  // WS Service Call ID <-> WS Clients [protected by mutex_sc2c_]
  // Keeps track of which client sent which service request so we can respond asynchronously.
  // NOTE: This will not be used if services are configured to called synchronous.
  // Assumptions:
  // - One service call ID, one client and vice versa
  // - Call IDs are unique (TODO(mfehr): probably not a great idea because the caller can set it!)
  // - Client can one-sided hang up asynchronously and invalidate their handle, hence lookups can fail.
public:
  bool hasCallIdToClientMapping(const uint32_t& call_id) const;
  void addCallIdToClientMapping(const uint32_t& call_id, WsServerClientHandle client_handle,
                                const std::string& client_name);
  void removeCallIdToClientMapping(const uint32_t& call_id);
  std::optional<ClientHandleWithName> getClientForCallId(const uint32_t& call_id) const;

  std::string callIdToClientMappingToString() const;

private:
  void cleanUpCallIdToClientMapping() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_sc2c_);

  using CallIdToClientMap = std::unordered_map<uint32_t, ClientHandleWithName>;
  mutable absl::Mutex mutex_sc2c_;
  CallIdToClientMap call_id_to_client_map_ ABSL_GUARDED_BY(mutex_sc2c_);
};

}  // namespace heph::ws_bridge
