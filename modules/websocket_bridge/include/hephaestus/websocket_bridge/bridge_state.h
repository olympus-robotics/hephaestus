//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/websocket_bridge/utils/ws_protocol.h"

namespace heph::ws {

class WebsocketBridgeState {
public:
  WebsocketBridgeState() = default;

  // Full State [protected by all mutexes]
public:
  auto toString() const -> std::string;
  void printBridgeState() const;

  [[nodiscard]] auto checkConsistency() const -> bool;

  // IPC Topics <-> WS Channels [protected by mutex_t2c_]
  // Keeps track of which IPC topic maps to which WS channel.
  // Assumptions:
  // - One IPC topic, one WS channel and vice versa
  // - Topic names and channel IDs are unique
public:
  auto getIpcTopicForWsChannel(const WsChannelId& channel_id) const -> std::string;
  auto getWsChannelForIpcTopic(const std::string& topic) const -> WsChannelId;
  void addWsChannelToIpcTopicMapping(const WsChannelId& channel_id, const std::string& topic);
  void removeWsChannelToIpcTopicMapping(const WsChannelId& channel_id, const std::string& topic);
  auto hasWsChannelMapping(const WsChannelId& channel_id) const -> bool;
  auto hasIpcTopicMapping(const std::string& topic) const -> bool;

  auto topicChannelMappingToString() const -> std::string;

private:
  using ChannelToTopicMap = std::unordered_map<WsChannelId, std::string>;
  using TopicToChannelMap = std::unordered_map<std::string, WsChannelId>;

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
  auto hasWsChannelWithClients(const WsChannelId& channel_id) const -> bool;
  void addWsChannelToClientMapping(const WsChannelId& channel_id, const WsClientHandle& client_handle,
                                   const std::string& client_name);
  void removeWsChannelToClientMapping(const WsChannelId& channel_id);
  void removeWsChannelToClientMapping(const WsChannelId& channel_id, const WsClientHandle& client_handle);
  auto getClientsForWsChannel(const WsChannelId& channel_id) const -> std::optional<WsClientHandleSet>;

private:
  void cleanUpChannelToClientMapping() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_c2c_);

  using ChannelToClientMap = std::unordered_map<WsChannelId, WsClientHandleSet>;
  mutable absl::Mutex mutex_c2c_;
  ChannelToClientMap channel_to_client_map_ ABSL_GUARDED_BY(mutex_c2c_);

  // IPC Services <-> WS Service [protected by mutex_s2s_]
  // Keeps track of which IPC service is mapped to which WS service.
  // Assumptions:
  // - One IPC service, one WS service and vice versa
  // - Service names and service IDs are unique
public:
  auto getIpcServiceForWsService(const WsServiceId& service_id) const -> std::string;
  auto getWsServiceForIpcService(const std::string& service_name) const -> WsChannelId;
  void addWsServiceToIpcServiceMapping(const WsServiceId& service_id, const std::string& service_name);
  void removeWsServiceToIpcServiceMapping(const WsServiceId& service_id, const std::string& service_name);
  auto hasWsServiceMapping(const WsChannelId& service_id) const -> bool;
  auto hasIpcServiceMapping(const std::string& service_name) const -> bool;
  auto servicMappingToString() const -> std::string;

private:
  using ServiceNameToServiceIdMap = std::unordered_map<std::string, WsServiceId>;
  using ServiceIdToServiceNameMap = std::unordered_map<WsServiceId, std::string>;

  mutable absl::Mutex mutex_s2s_;

  ServiceNameToServiceIdMap service_name_to_service_id_map_ ABSL_GUARDED_BY(mutex_s2s_);
  ServiceIdToServiceNameMap service_id_to_service_name_map_ ABSL_GUARDED_BY(mutex_s2s_);

  // WS Service Call ID <-> WS Clients [protected by mutex_sc2c_]
  // Keeps track of which client sent which service request so we can respond asynchronously.
  // NOTE: This will not be used if services are configured to called synchronous.
  // Assumptions:
  // - One service call ID, one client and vice versa
  // - Call IDs are unique (TODO: probably not a great idea because we are at the  mercy of the caller!)
  // - Client can one-sided hang up asynchronously and invalidate their handle, hence lookups can fail.
public:
  auto hasCallIdToClientMapping(uint32_t call_id) const -> bool;
  void addCallIdToClientMapping(uint32_t call_id, const WsClientHandle& client_handle,
                                const std::string& client_name);
  void removeCallIdToClientMapping(uint32_t call_id);
  auto getClientForCallId(uint32_t call_id) const -> std::optional<ClientHandleWithName>;

  auto callIdToClientMappingToString() const -> std::string;

private:
  void cleanUpCallIdToClientMapping() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_sc2c_);

  using CallIdToClientMap = std::unordered_map<uint32_t, ClientHandleWithName>;
  mutable absl::Mutex mutex_sc2c_;
  CallIdToClientMap call_id_to_client_map_ ABSL_GUARDED_BY(mutex_sc2c_);

  // WS Client Channel ID <-> IPC Topic [protected by mutex_cc2t_]
  // Keeps track of which WS client-advertised channel maps to which IPC topic
  // Assumptions:
  // - one IPC topic can be served by multiple WS client-advertised channels
  // - IPC Topic names and WS client-advertised channel IDs are unique
public:
  auto hasClientChannelsForTopic(const std::string& topic) const -> bool;
  auto getTopicForClientChannel(const WsClientChannelId& client_channel_id) const -> std::string;
  auto getClientChannelsForTopic(const std::string& topic) const -> WsClientChannelIdSet;
  void addClientChannelToTopicMapping(const WsClientChannelId& client_channel_id, const std::string& topic);
  void removeClientChannelToTopicMapping(const WsClientChannelId& client_channel_id);
  auto hasClientChannelMapping(const WsClientChannelId& client_channel_id) const -> bool;
  auto hasTopicToClientChannelMapping(const std::string& topic) const -> bool;

  auto clientChannelMappingToString() const -> std::string;

private:
  using ClientChannelToTopicMap = std::unordered_map<WsClientChannelId, std::string>;
  using TopicToClientChannelMap = std::unordered_map<std::string, WsClientChannelIdSet>;

  mutable absl::Mutex mutex_cc2t_;
  ClientChannelToTopicMap client_channel_to_topic_ ABSL_GUARDED_BY(mutex_cc2t_);
  TopicToClientChannelMap topic_to_client_channels_ ABSL_GUARDED_BY(mutex_cc2t_);

  // WS Client Channels <-> WS Clients [protected by mutex_cc2c_]
  // Keeps track of which client channel was advertised by which client.
  // Assumptions:
  // - One WS client-advertised channel, One WS client
  // - IPC Topic names and WS client-advertised channel IDs are unique
  // - WS Client can one-sided hang up asynchronously and invalidate their handle, hence lookups can fail.
public:
  auto hasClientForClientChannel(const WsClientChannelId& client_channel_id) const -> bool;
  void addClientChannelToClientMapping(const WsClientChannelId& client_channel_id,
                                       const WsClientHandle& client_handle, const std::string& client_name);
  void removeClientChannelToClientMapping(const WsClientChannelId& client_channel_id);
  auto getClientForClientChannel(const WsClientChannelId& client_channel_id) const
      -> std::optional<ClientHandleWithName>;

private:
  void cleanUpClientChannelToClientMapping() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_cc2c_);

  using ClientChannelToClientMap = std::unordered_map<WsClientChannelId, ClientHandleWithName>;
  mutable absl::Mutex mutex_cc2c_;
  ClientChannelToClientMap client_channel_to_client_map_ ABSL_GUARDED_BY(mutex_cc2c_);
};

}  // namespace heph::ws
