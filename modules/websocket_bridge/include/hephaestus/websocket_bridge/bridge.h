//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <absl/log/check.h>
#include <absl/synchronization/mutex.h>
#include <fmt/base.h>
#include <fmt/core.h>
#include <foxglove/websocket/callback_queue.hpp>
#include <foxglove/websocket/regex_utils.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/ipc/zenoh/session.h>
#include <magic_enum.hpp>
#include <websocketpp/common/connection_hdl.hpp>

#include "hephaestus/ipc/ipc_graph.h"
#include "hephaestus/ipc/ipc_interface.h"
#include "hephaestus/utils/ws_protocol.h"
#include "hephaestus/websocket_bridge/bridge_config.h"
#include "hephaestus/websocket_bridge/bridge_state.h"

namespace heph::ws {

class WsBridge {
public:
  WsBridge(const std::shared_ptr<ipc::zenoh::Session>& session, const WsBridgeConfig& config);

  ~WsBridge() = default;

  void start();
  void stop();

private:
  WsBridgeConfig config_;

  WsBridgeState state_;

  ////////////////////////////////
  // Websocket Server Interface //
  ////////////////////////////////

  WsServerInterfacePtr ws_server_;

  // Callbacks triggered by WS Server
  // NOLINTBEGIN(readability-identifier-naming)
  static void callback_Ws_Log(WsServerLogLevel level, char const* msg);
  void callback_Ws_Subscribe(WsServerChannelId channel_id, const WsServerClientHandle& client_handle);
  void callback_Ws_Unsubscribe(WsServerChannelId channel_id, const WsServerClientHandle& client_handle);
  void callback_WsServer_ClientAdvertise(const WsServerClientChannelAd& advertisement,
                                         const WsServerClientHandle& client_handle);
  void callback_Ws_ClientUnadvertise(WsServerClientChannelId client_channel_id,
                                     const WsServerClientHandle& client_handle);
  void callback_Ws_ClientMessage(const WsServerClientMessage& message,
                                 const WsServerClientHandle& client_handle);
  void callback_Ws_ServiceRequest(const WsServerServiceRequest& request,
                                  const WsServerClientHandle& client_handle);

  void callback_Ws_SubscribeConnectionGraph(bool subscribe);
  // NOLINTEND(readability-identifier-naming)

  ///////////////////
  // IPC Interface //
  ///////////////////

  std::unique_ptr<IpcGraph> ipc_graph_;

  // Callbacks triggered by the IPC Graph
  // NOLINTBEGIN(readability-identifier-naming)
  void callback_IpcGraph_TopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info);
  void callback_IpcGraph_TopicDropped(const std::string& topic);

  void callback_IpcGraph_ServiceFound(const std::string& service,
                                      const heph::serdes::ServiceTypeInfo& type_info);
  void callback_IpcGraph_ServiceDropped(const std::string& service);

  void callback_IpcGraph_Updated(const ipc::zenoh::EndpointInfo& info, IpcGraphState ipc_graph_state);
  // NOLINTEND(readability-identifier-naming)

  std::unique_ptr<IpcInterface> ipc_interface_;

  // Callbacks triggered by the IPC interface
  // NOLINTBEGIN(readability-identifier-naming)
  void callback_Ipc_MessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
                                    std::span<const std::byte> data, const heph::serdes::TypeInfo& type_info);

  void callback_Ipc_ServiceResponsesReceived(
      WsServerServiceId service_id, WsServerServiceCallId call_id, const RawServiceResponses& responses,
      std::optional<ClientHandleWithName> client_handle_w_name_opt = std::nullopt);
  // NOLINTEND(readability-identifier-naming)
};

}  // namespace heph::ws
