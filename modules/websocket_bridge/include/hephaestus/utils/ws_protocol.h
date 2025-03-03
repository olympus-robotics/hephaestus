//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/server_interface.hpp>
#include <hephaestus/ipc/zenoh/service.h>
#include <nlohmann/json_fwd.hpp>
#include <websocketpp/common/connection_hdl.hpp>

#include "hephaestus/utils/protobuf_serdes.h"

namespace heph::ws {

using WsServerClientHandle = websocketpp::connection_hdl;
using WsServerInterface = foxglove::ServerInterface<WsServerClientHandle>;
using WsServerInterfacePtr = std::unique_ptr<WsServerInterface>;
using WsServerHandlers = foxglove::ServerHandlers<WsServerClientHandle>;
using WsServerFactory = foxglove::ServerFactory;
using WsServerInfo = foxglove::ServerOptions;
using WsServerLogLevel = foxglove::WebSocketLogLevel;

using WsServerChannelId = foxglove::ChannelId;
using WsServerChannelInfo = foxglove::ChannelWithoutId;
using WsServerChannelAd = foxglove::Channel;

using WsServerClientChannelId = foxglove::ClientChannelId;
using WsServerClientChannelIdSet = std::unordered_set<WsServerClientChannelId>;
using WsServerClientChannelAd = foxglove::ClientAdvertisement;
using WsServerSubscriptionId = foxglove::SubscriptionId;
using WsServerClientMessage = foxglove::ClientMessage;

using WsServerServiceId = foxglove::ServiceId;
using WsServerServiceCallId = uint32_t;
using WsServerServiceInfo = foxglove::ServiceWithoutId;
using WsServerServiceAd = foxglove::Service;
using WsServerServiceRequestDefinition = foxglove::ServiceRequestDefinition;
using WsServerServiceResponseDefinition = foxglove::ServiceResponseDefinition;
using WsServerServiceRequest = foxglove::ServiceRequest;
using WsServerServiceResponse = foxglove::ServiceResponse;

using WsServerBinaryOpCode = foxglove::BinaryOpcode;
using WsServerClientBinaryOpCode = foxglove::ClientBinaryOpcode;

using ClientHandleWithName = std::pair<WsServerClientHandle, std::string>;

struct WsServerClientComparator {
  auto operator()(const ClientHandleWithName& lhs, const ClientHandleWithName& rhs) const -> bool {
    return lhs.first.lock() < rhs.first.lock();
  }
};
using WsServerClientHandleSet = std::set<ClientHandleWithName, WsServerClientComparator>;

auto convertIpcRawServiceResponseToWsServiceResponse(
    WsServerServiceId service_id, WsServerServiceCallId call_id,
    const ipc::zenoh::ServiceResponse<std::vector<std::byte>>& raw_response,
    WsServerServiceResponse& ws_response) -> bool;

[[nodiscard]] auto convertWsJsonMsgToChannel(const nlohmann::json& channel_json)
    -> std::optional<WsServerChannelAd>;
[[nodiscard]] auto convertWsJsonMsgtoServerOptions(const nlohmann::json& server_options_json)
    -> std::optional<WsServerInfo>;
[[nodiscard]] auto convertWsJsonMsgToService(const nlohmann::json& service_json)
    -> std::optional<WsServerServiceAd>;

struct WsServerAdvertisements {
  WsServerInfo info;
  std::unordered_map<WsServerChannelId, WsServerChannelAd> channels;
  std::unordered_map<WsServerServiceId, WsServerServiceAd> services;

  ProtobufSchemaDatabase schema_db;
};

auto parseWsServerAdvertisements(const nlohmann::json& server_txt_msg, WsServerAdvertisements& ws_server_ads)
    -> bool;

struct WsServerServiceFailure {
  WsServerServiceCallId call_id;
  std::string error_message;
};

auto parseWsServerServiceFailure(const nlohmann::json& server_txt_msg,
                                 WsServerServiceFailure& service_failure) -> bool;

}  // namespace heph::ws