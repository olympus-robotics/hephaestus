//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <absl/log/check.h>
#include <absl/synchronization/mutex.h>
#include <fmt/base.h>
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

#include "hephaestus/ipc/ipc_graph.h"
#include "hephaestus/ipc/ipc_interface.h"
#include "hephaestus/websocket_bridge/bridge_config.h"
#include "hephaestus/websocket_bridge/bridge_state.h"
#include "hephaestus/utils/protobuf_serdes.h"
#include "hephaestus/utils/ws_protocol.h"

namespace heph::ws {

class WsBridge {
public:
  WsBridge(std::shared_ptr<ipc::zenoh::Session> session, const WsBridgeConfig& config);
  ~WsBridge();

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
  void callback__WsServer__Log(WsServerLogLevel level, char const* msg);
  void callback__WsServer__Subscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void callback__WsServer__Unsubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void callback__WsServer__ClientAdvertise(const WsServerClientChannelAd& advertisement,
                                           WsServerClientHandle client_handle);
  void callback__WsServer__ClientUnadvertise(WsServerClientChannelId client_channel_id,
                                             WsServerClientHandle client_handle);
  void callback__WsServer__ClientMessage(const WsServerClientMessage& message,
                                         WsServerClientHandle client_handle);
  void callback__WsServer__ServiceRequest(const WsServerServiceRequest& request,
                                          WsServerClientHandle client_handle);

  void callback__WsServer__SubscribeConnectionGraph(bool subscribe);

  ///////////////////
  // IPC Interface //
  ///////////////////

  std::unique_ptr<IpcGraph> ipc_graph_;

  // Callbacks triggered by the IPC Graph
  void callback__IpcGraph__TopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info);
  void callback__IpcGraph__TopicDropped(const std::string& topic);

  void callback__IpcGraph__ServiceFound(const std::string& service,
                                        const heph::serdes::ServiceTypeInfo& type_info);
  void callback__IpcGraph__ServiceDropped(const std::string& service);

  void callback__IpcGraph__Updated(const ipc::zenoh::EndpointInfo& info, IpcGraphState ipc_graph_state);

  std::unique_ptr<IpcInterface> ipc_interface_;

  // Callbacks triggered by the IPC interface
  void callback__Ipc__MessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
                                      std::span<const std::byte> data,
                                      const heph::serdes::TypeInfo& type_info);

  void callback__Ipc__ServiceResponsesReceived(
      WsServerServiceId service_id, WsServerServiceCallId call_id, const RawServiceResponses& responses,
      std::optional<ClientHandleWithName> client_handle_opt = std::nullopt);
};

}  // namespace heph::ws
