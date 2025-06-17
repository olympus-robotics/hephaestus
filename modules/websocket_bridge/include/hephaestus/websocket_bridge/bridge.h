//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "hephaestus/ipc/zenoh/ipc_graph.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/websocket_bridge/bridge_config.h"
#include "hephaestus/websocket_bridge/bridge_state.h"
#include "hephaestus/websocket_bridge/ipc/ipc_entity_manager.h"
#include "hephaestus/websocket_bridge/utils/ws_protocol.h"

namespace heph::ws {

class WebsocketBridge {
public:
  WebsocketBridge(const std::shared_ptr<ipc::zenoh::Session>& session, const WebsocketBridgeConfig& config);

  ~WebsocketBridge() = default;

  void start();
  void stop();

private:
  WebsocketBridgeConfig config_;

  WebsocketBridgeState state_;

private:
  ////////////////////////////////
  // Websocket Server Interface //
  ////////////////////////////////

  // Callbacks triggered by WS Server
  // NOLINTBEGIN(readability-identifier-naming)
  static void callback_Ws_Log(WsLogLevel level, const char* msg);
  void callback_Ws_Subscribe(WsChannelId channel_id, const WsClientHandle& client_handle);
  void callback_Ws_Unsubscribe(WsChannelId channel_id, const WsClientHandle& client_handle);
  void callback_Ws_ClientAdvertise(const WsClientChannelAd& advertisement,
                                   const WsClientHandle& client_handle);
  void callback_Ws_ClientUnadvertise(WsClientChannelId client_channel_id,
                                     const WsClientHandle& client_handle);
  void callback_Ws_ClientMessage(const WsClientMessage& message, const WsClientHandle& client_handle);
  void callback_Ws_ServiceRequest(const WsServiceRequest& request, const WsClientHandle& client_handle);

  void callback_Ws_SubscribeConnectionGraph(bool subscribe);
  // NOLINTEND(readability-identifier-naming)

  ///////////////////
  // IPC Interface //
  ///////////////////

  using IpcGraph = ipc::zenoh::IpcGraph;
  using IpcGraphState = ipc::zenoh::IpcGraphState;
  using IpcGraphCallbacks = ipc::zenoh::IpcGraphCallbacks;
  using IpcGraphConfig = ipc::zenoh::IpcGraphConfig;
  using TopicsToTypeMap = ipc::zenoh::TopicsToTypeMap;
  using TopicsToServiceTypesMap = ipc::zenoh::TopicsToServiceTypesMap;
  using TopicToSessionIdMap = ipc::zenoh::TopicToSessionIdMap;

  // Callbacks triggered by the IPC Graph
  // NOLINTBEGIN(readability-identifier-naming)
  void callback_IpcGraph_TopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info);
  void callback_IpcGraph_TopicDropped(const std::string& topic);

  void callback_IpcGraph_ServiceFound(const std::string& service,
                                      const heph::serdes::ServiceTypeInfo& type_info);
  void callback_IpcGraph_ServiceDropped(const std::string& service);

  void callback_IpcGraph_Updated(const ipc::zenoh::EndpointInfo& info, IpcGraphState ipc_graph_state);
  // NOLINTEND(readability-identifier-naming)

  // Callbacks triggered by the IPC interface
  // NOLINTBEGIN(readability-identifier-naming)
  void callback_Ipc_MessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
                                    std::span<const std::byte> data, const heph::serdes::TypeInfo& type_info);

  void callback_Ipc_ServiceResponsesReceived(
      WsServiceId service_id, WsServiceCallId call_id, const RawServiceResponses& responses,
      std::optional<ClientHandleWithName> client_handle_w_name_opt = std::nullopt);
  // NOLINTEND(readability-identifier-naming)

private:
  WsInterfacePtr ws_server_;

  std::unique_ptr<IpcGraph> ipc_graph_;

  std::unique_ptr<IpcEntityManager> ipc_entity_manager_;
};

}  // namespace heph::ws
