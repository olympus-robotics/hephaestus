#pragma once

#include <memory>
#include <set>
#include <unordered_map>

#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/ipc/zenoh/service.h>
#include <nlohmann/json.hpp>
#include <websocketpp/common/connection_hdl.hpp>

#include "hephaestus/websocket_bridge/bridge_config.h"
#include "hephaestus/websocket_bridge/protobuf_utils.h"

namespace heph::ws_bridge {

using WsServerClientHandle = websocketpp::connection_hdl;
using WsServerInterface = foxglove::ServerInterface<WsServerClientHandle>;
using WsServerInterfacePtr = std::unique_ptr<WsServerInterface>;
using WsServerLogLevel = foxglove::WebSocketLogLevel;

using WsServerChannelId = foxglove::ChannelId;
using WsServerChannelInfo = foxglove::ChannelWithoutId;
using WsServerChannelAd = foxglove::Channel;

using WsServerServiceId = foxglove::ServiceId;
using WsServerServiceCallId = uint32_t;
using WsServerServiceInfo = foxglove::ServiceWithoutId;
using WsServerServiceAd = foxglove::Service;
using WsServerServiceDefinition = foxglove::ServiceRequestDefinition;
using WsServerServiceRequest = foxglove::ServiceRequest;
using WsServerServiceResponse = foxglove::ServiceResponse;

using WsServerInfo = foxglove::ServerOptions;

using ClientHandleWithName = std::pair<WsServerClientHandle, std::string>;

struct WsServerClientComparator {
  bool operator()(const ClientHandleWithName& lhs, const ClientHandleWithName& rhs) const {
    return lhs.first.lock() < rhs.first.lock();
  }
};
using WsServerClientHandleSet = std::set<ClientHandleWithName, WsServerClientComparator>;

bool convertIpcRawServiceResponseToWsServiceResponse(
    WsServerServiceId service_id, WsServerServiceCallId call_id,
    const ipc::zenoh::ServiceResponse<std::vector<std::byte>>& raw_response,
    WsServerServiceResponse& ws_response);

std::optional<WsServerChannelAd> convertWsJsonMsgToChannel(const nlohmann::json& channel_json);
std::optional<WsServerInfo> convertWsJsonMsgtoServerOptions(const nlohmann::json& server_options_json);
std::optional<WsServerServiceAd> convertWsJsonMsgToService(const nlohmann::json& service_json);

struct WsServerAdvertisements {
  WsServerInfo info;
  std::unordered_map<WsServerChannelId, WsServerChannelAd> channels;
  std::unordered_map<WsServerServiceId, WsServerServiceAd> services;

  ProtobufSchemaDatabase schema_db;
};

bool parseWsServerAdvertisements(const nlohmann::json& server_txt_msg, WsServerAdvertisements& ws_server_ads);

struct WsServerServiceFailure {
  WsServerServiceCallId call_id;
  std::string error_message;
};

bool parseWsServerServiceFailure(const nlohmann::json& server_txt_msg,
                                 WsServerServiceFailure& service_failure);

}  // namespace heph::ws_bridge