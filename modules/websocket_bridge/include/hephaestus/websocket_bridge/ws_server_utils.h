#pragma once

#include <memory>
#include <set>
#include <unordered_map>

#include <foxglove/websocket/server_interface.hpp>
#include <websocketpp/common/connection_hdl.hpp>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws_bridge {

using WsServerClientHandle = websocketpp::connection_hdl;
using WsServerInterface = foxglove::ServerInterface<WsServerClientHandle>;
using WsServerInterfacePtr = std::unique_ptr<WsServerInterface>;
using WsServerLogLevel = foxglove::WebSocketLogLevel;

using WsServerChannelId = foxglove::ChannelId;
using WsServerChannelInfo = foxglove::ChannelWithoutId;

using WsServerServiceId = foxglove::ServiceId;
using WsServerServiceInfo = foxglove::ServiceWithoutId;
using WsServerServiceDefinition = foxglove::ServiceRequestDefinition;
using WsServerServiceRequest = foxglove::ServiceRequest;
using WsServerServiceResponse = foxglove::ServiceRequest;

using ClientHandleWithName = std::pair<WsServerClientHandle, std::string>;

struct WsServerClientComparator {
  bool operator()(const ClientHandleWithName& lhs, const ClientHandleWithName& rhs) const {
    return lhs.first.lock() < rhs.first.lock();
  }
};
using WsServerClientHandleSet = std::set<ClientHandleWithName, WsServerClientComparator>;

}  // namespace heph::ws_bridge