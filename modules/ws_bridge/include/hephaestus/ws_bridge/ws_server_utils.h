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

struct WsServerClientComparator {
  bool operator()(const WsServerClientHandle& lhs, const WsServerClientHandle& rhs) const {
    return lhs.lock() < rhs.lock();
  }
};

using WsServerClientHandleSet = std::set<WsServerClientHandle, WsServerClientComparator>;

using WsChannelIdToClientHandleMap = std::unordered_map<foxglove::ChannelId, WsServerClientHandleSet>;

foxglove::ServerOptions GetWsServerOptions(const BridgeConfig& config);

}  // namespace heph::ws_bridge