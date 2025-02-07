//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
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
#include "hephaestus/ws_bridge/ws_bridge_state.h"
#include "hephaestus/ws_bridge/ws_server_utils.h"

namespace heph::ws_bridge {

class WsBridge {
public:
  WsBridge(std::shared_ptr<ipc::zenoh::Session> session, const WsBridgeConfig& config);
  ~WsBridge();

  [[nodiscard]] auto start() -> std::future<void>;
  [[nodiscard]] auto stop() -> std::future<void>;
  void wait() const;

private:
  mutable absl::Mutex mutex_;

  WsBridgeConfig config_;

  std::unique_ptr<concurrency::Spinner> spinner_;

  WsBridgeState state_;

  ////////////////////////////////
  // Websocket Server Interface //
  ////////////////////////////////

  WsServerInterfacePtr ws_server_;

  // WS Server - Connection Graph
  bool ws_server_subscribed_to_connection_graph_{ false };

  // Callbacks triggered by WS Server [THREADSAFE]
  void callback__WsServer__Log(WsServerLogLevel level, char const* msg);
  void callback__WsServer__Subscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void callback__WsServer__Unsubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle);
  void callback__WsServer__ClientAdvertise(const foxglove::ClientAdvertisement& advertisement,
                                           WsServerClientHandle client_handle);
  void callback__WsServer__ClientUnadvertise(WsServerChannelId channel_type,
                                             WsServerClientHandle client_handle);
  void callback__WsServer__ClientMessage(const foxglove::ClientMessage& message,
                                         WsServerClientHandle client_handle);
  void callback__WsServer__ServiceRequest(const foxglove::ServiceRequest& request,
                                          WsServerClientHandle client_handle);
  void callback__WsServer__SubscribeConnectionGraph(bool subscribe);

  ///////////////////
  // IPC Interface //
  ///////////////////

  std::unique_ptr<IpcGraph> ipc_graph_;

  // Callbacks triggered by the IPC Graph [THREADSAFE]
  void callback__IpcGraph__TopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info);
  void callback__IpcGraph__TopicDropped(const std::string& topic);
  void callback__IpcGraph__Updated(IpcGraphState ipc_graph_state);

  // std::unique_ptr<IpcInterface> ipc_interface_;

  // // Callbacks triggered by the IPC interface [THREADSAFE]
  // void callback__Ipc__MessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
  //                                     std::span<const std::byte> data, const heph::serdes::TypeInfo&
  //                                     type_info);
};

}  // namespace heph::ws_bridge
