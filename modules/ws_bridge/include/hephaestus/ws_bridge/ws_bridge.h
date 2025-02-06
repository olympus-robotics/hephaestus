//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <absl/log/check.h>
#include <fmt/core.h>
#include <foxglove/websocket/callback_queue.hpp>
#include <magic_enum.hpp>
// #include <foxglove_bridge/foxglove/websocket.hpp>
// #include <foxglove_bridge/message_definition_cache.hpp>
#include <foxglove/websocket/regex_utils.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/ipc/zenoh/session.h>
#include <websocketpp/common/connection_hdl.hpp>

#include "hephaestus/ws_bridge/config.h"
#include "hephaestus/ws_bridge/ipc_graph.h"
#include "hephaestus/ws_bridge/ws_server_utils.h"

namespace heph::ws_bridge {

class Bridge {
public:
public:
  Bridge(std::shared_ptr<ipc::zenoh::Session> session, const BridgeConfig& config);
  ~Bridge();

  [[nodiscard]] auto start() -> std::future<void>;
  [[nodiscard]] auto stop() -> std::future<void>;
  void wait() const;

private:
  mutable std::mutex mutex_;

  BridgeConfig config_;

  ////////////////////////////////
  // Websocket Server Interface //
  ////////////////////////////////

  WsServerInterfacePtr ws_server_;

  void StartWsServer(const BridgeConfig& config);
  void StopWsServer();

  // WS Server - Connection Graph
  bool ws_server_subscribed_to_connection_graph_{ false };

  void UpdateWsServerConnectionGraph(const TopicsToTypesMap& topics_w_type,
                                     const TopicsToTypesMap& services_to_nodes,
                                     const TopicToNodesMap& topic_to_subs,
                                     const TopicToNodesMap& topic_to_pubs);

  // WS Server - Channel Id to Client Mapping
  // bool HasWsChannelWithClients(const WsServerChannelId& channel_id) const;
  // void AddWsChannelToClientMapping(const WsServerChannelId& channel_id, WsServerClientHandle
  // client_handle); void CleanUpChannelToClientMapping(); void RemoveWsChannelToClientMapping(const
  // WsServerChannelId& channel_id,
  //                                     WsServerClientHandle client_handle);
  // void RemoveWsChannelToClientMapping(const WsServerChannelId& channel_id);
  // std::optional<WsServerClientHandleSet> GetClientsForWsChannel(const WsServerChannelId& channel_id) const;
  // std::string GetChannelClientMappingAsString() const;

  // WsChannelIdToClientHandleMap ws_server_channel_to_client_map_;

  // WS Server - Callbacks triggered by the server [THREADSAFE]
  void CallbackWsServerLogHandler(WsServerLogLevel level, char const* msg);
  void CallbackWsServerSubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void CallbackWsServerUnsubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void CallbackWsServerClientAdvertise(const foxglove::ClientAdvertisement& advertisement,
                                       WsServerClientHandle client_handle);
  void CallbackWsServerClientUnadvertise(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void CallbackWsServerClientMessage(const foxglove::ClientMessage& message,
                                     WsServerClientHandle client_handle);
  void CallbackWsServerServiceRequest(const foxglove::ServiceRequest& request,
                                      WsServerClientHandle client_handle);
  void CallbackWsServerSubscribeConnectionGraph(bool subscribe);

  ///////////////////
  // IPC Interface //
  ///////////////////

  std::unique_ptr<IpcGraph> ipc_graph_;

  // Callbacks triggered by the IPC Graph [THREADSAFE]
  void CallbackIpcGraphTopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info);
  void CallbackIpcGraphTopicDropped(const std::string& topic);
  void CallbackIpcGraphUpdated(IpcGraphState ipc_graph_state);

  // std::unique_ptr<IpcInterface> ipc_interface_;

  // // Callbacks triggered by the IPC interface [THREADSAFE]
  // void CallbackIpcMessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
  //                                 std::span<const std::byte> data, const heph::serdes::TypeInfo&
  //                                 type_info);
  // void CallbackPrintBridgeStatus() const;

  ////////////////////////////////
  // [WS Server <=> IPC] Bridge //
  ////////////////////////////////

  // Bidirectional WS Channel to IPC Topic Mapping
  // std::string GetIpcTopicForWsChannel(const WsServerChannelId& channel_id) const;
  // WsServerChannelId GetWsChannelForIpcTopic(const std::string& topic) const;
  // void AddWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id, const std::string& topic);
  // void RemoveWsChannelToIpcTopicMapping(const WsServerChannelId& channel_id, const std::string& topic);
  // bool HasWsChannelMapping(const WsServerChannelId& channel_id) const;
  // bool HasIpcTopicMapping(const std::string& topic) const;
  // std::string GetTopicChannelMappingAsString() const;
  // std::unordered_map<foxglove::ChannelId, std::string> ws_server_channel_id_to_ipc_topic_map_;
  // std::unordered_map<std::string, foxglove::ChannelId> ws_server_ipc_topic_to_channel_id_map_;

  // std::string BridgeStatusAsString() const;
};

}  // namespace heph::ws_bridge
