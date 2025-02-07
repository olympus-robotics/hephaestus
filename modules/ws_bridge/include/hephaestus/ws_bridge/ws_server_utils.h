#pragma once

#include <memory>
#include <set>
#include <unordered_map>

#include <foxglove/websocket/server_interface.hpp>
#include <websocketpp/common/connection_hdl.hpp>

#include "hephaestus/ws_bridge/config.h"

namespace heph::ws_bridge {

using WsServerClientHandle = websocketpp::connection_hdl;
using WsServerInterface = foxglove::ServerInterface<WsServerClientHandle>;
using WsServerInterfacePtr = std::unique_ptr<WsServerInterface>;
using WsServerLogLevel = foxglove::WebSocketLogLevel;

using WsServerChannelId = foxglove::ChannelId;
using WsServerChannelInfo = foxglove::ChannelWithoutId;

using WsServerServiceId = foxglove::ServiceId;
using WsServerServiceInfo = foxglove::ServiceWithoutId;

using ClientHandleWithName = std::pair<WsServerClientHandle, std::string>;

struct WsServerClientComparator {
  bool operator()(const ClientHandleWithName& lhs, const ClientHandleWithName& rhs) const {
    return lhs.first.lock() < rhs.first.lock();
  }
};
using WsServerClientHandleSet = std::set<ClientHandleWithName, WsServerClientComparator>;

foxglove::ServerOptions GetWsServerOptions(const WsBridgeConfig& config);

}  // namespace heph::ws_bridge