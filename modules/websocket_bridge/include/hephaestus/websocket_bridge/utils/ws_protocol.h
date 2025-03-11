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

#include "hephaestus/websocket_bridge/utils/protobuf_serdes.h"

namespace heph::ws {

using WsClientHandle = foxglove::ConnHandle;
using WsInterface = foxglove::ServerInterface<WsClientHandle>;
using WsInterfacePtr = std::unique_ptr<WsInterface>;
using WsHandlers = foxglove::ServerHandlers<WsClientHandle>;
using WsFactory = foxglove::ServerFactory;
using WsInfo = foxglove::ServerOptions;
using WsLogLevel = foxglove::WebSocketLogLevel;

using WsChannelId = foxglove::ChannelId;
using WsChannelInfo = foxglove::ChannelWithoutId;
using WsChannelAd = foxglove::Channel;

using WsClientChannelId = foxglove::ClientChannelId;
using WsClientChannelIdSet = std::unordered_set<WsClientChannelId>;
using WsClientChannelAd = foxglove::ClientAdvertisement;
using WsSubscriptionId = foxglove::SubscriptionId;
using WsClientMessage = foxglove::ClientMessage;

using WsServiceId = foxglove::ServiceId;
using WsServiceCallId = uint32_t;
using WsServiceInfo = foxglove::ServiceWithoutId;
using WsServiceAd = foxglove::Service;
using WsServiceRequestDefinition = foxglove::ServiceRequestDefinition;
using WsServiceResponseDefinition = foxglove::ServiceResponseDefinition;
using WsServiceRequest = foxglove::ServiceRequest;
using WsServiceResponse = foxglove::ServiceResponse;

using WsBinaryOpCode = foxglove::BinaryOpcode;
using WsClientBinaryOpCode = foxglove::ClientBinaryOpcode;

using ClientHandleWithName = std::pair<WsClientHandle, std::string>;

struct WsClientComparator {
  auto operator()(const ClientHandleWithName& lhs, const ClientHandleWithName& rhs) const -> bool {
    return lhs.first.lock() < rhs.first.lock();
  }
};
using WsClientHandleSet = std::set<ClientHandleWithName, WsClientComparator>;

auto convertIpcRawServiceResponseToWsServiceResponse(
    WsServiceId service_id, WsServiceCallId call_id,
    const ipc::zenoh::ServiceResponse<std::vector<std::byte>>& raw_response, WsServiceResponse& ws_response)
    -> bool;

[[nodiscard]] auto convertWsJsonMsgToChannel(const nlohmann::json& channel_json)
    -> std::optional<WsChannelAd>;
[[nodiscard]] auto convertWsJsonMsgtoServerOptions(const nlohmann::json& server_options_json)
    -> std::optional<WsInfo>;
[[nodiscard]] auto convertWsJsonMsgToService(const nlohmann::json& service_json)
    -> std::optional<WsServiceAd>;

struct WsAdvertisements {
  WsInfo info;
  std::unordered_map<WsChannelId, WsChannelAd> channels;
  std::unordered_map<WsServiceId, WsServiceAd> services;

  ProtobufSchemaDatabase schema_db;
};

auto parseWsAdvertisements(const nlohmann::json& server_txt_msg, WsAdvertisements& ws_server_ads) -> bool;

struct WsServiceFailure {
  WsServiceCallId call_id;
  std::string error_message;
};

auto parseWsServiceFailure(const nlohmann::json& server_txt_msg, WsServiceFailure& service_failure) -> bool;

}  // namespace heph::ws
