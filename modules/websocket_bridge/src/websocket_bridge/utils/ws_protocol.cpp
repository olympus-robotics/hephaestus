//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/utils/ws_protocol.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/websocket_bridge/utils/protobuf_serdes.h"

namespace heph::ws {

auto convertIpcRawServiceResponseToWsServiceResponse(
    WsServiceId service_id, WsServiceCallId call_id,
    const ipc::zenoh::ServiceResponse<std::vector<std::byte>>& raw_response, WsServiceResponse& ws_response)
    -> bool {
  if (raw_response.value.empty()) {
    return false;
  }

  std::vector<uint8_t> response_data(raw_response.value.size());
  std::ranges::transform(raw_response.value, response_data.begin(),
                         [](std::byte b) { return static_cast<uint8_t>(b); });

  ws_response = {
    .serviceId = service_id,
    .callId = call_id,
    .encoding = "protobuf",
    .data = std::move(response_data),
  };
  return true;
}

auto convertWsJsonMsgToChannel(const nlohmann::json& channel_json) -> std::optional<WsChannelAd> {
  try {
    WsChannelAd channel;
    channel.id = channel_json.at("id").get<uint32_t>();
    channel.topic = channel_json.at("topic").get<std::string>();
    channel.encoding = channel_json.at("encoding").get<std::string>();
    channel.schemaName = channel_json.at("schemaName").get<std::string>();
    if (channel_json.contains("schema")) {
      channel.schema = channel_json.at("schema").get<std::string>();
    }
    if (channel_json.contains("schemaEncoding")) {
      channel.schemaEncoding = channel_json.at("schemaEncoding").get<std::string>();
    }
    return channel;
  } catch (const nlohmann::json::exception&) {
    return std::nullopt;
  }
}

auto convertWsJsonMsgtoServerOptions(const nlohmann::json& server_options_json) -> std::optional<WsInfo> {
  try {
    WsInfo server_options;
    if (server_options_json.contains("capabilities")) {
      const auto& capabilities_json = server_options_json.at("capabilities");
      if (!capabilities_json.is_array()) {
        return std::nullopt;
      }
      for (const auto& capability : capabilities_json) {
        if (!capability.is_string()) {
          return std::nullopt;
        }
        server_options.capabilities.push_back(capability.get<std::string>());
      }
    }
    if (server_options_json.contains("metadata")) {
      server_options.metadata =
          server_options_json.at("metadata").get<std::unordered_map<std::string, std::string>>();
    }
    return server_options;
  } catch (const nlohmann::json::exception&) {
    return std::nullopt;
  }
}

auto convertWsJsonMsgToService(const nlohmann::json& service_json) -> std::optional<WsServiceAd> {
  try {
    WsServiceAd service;
    service.id = service_json.at("id").get<uint32_t>();
    service.name = service_json.at("name").get<std::string>();
    service.type = service_json.at("type").get<std::string>();
    if (service_json.contains("request")) {
      const auto& request_json = service_json.at("request");
      WsServiceRequestDefinition request_def;
      request_def.encoding = request_json.at("encoding").get<std::string>();
      request_def.schemaName = request_json.at("schemaName").get<std::string>();
      request_def.schemaEncoding = request_json.at("schemaEncoding").get<std::string>();
      request_def.schema = request_json.at("schema").get<std::string>();
      service.request = std::move(request_def);
    }
    if (service_json.contains("response")) {
      const auto& response_json = service_json.at("response");
      WsServiceResponseDefinition response_def;
      response_def.encoding = response_json.at("encoding").get<std::string>();
      response_def.schemaName = response_json.at("schemaName").get<std::string>();
      response_def.schemaEncoding = response_json.at("schemaEncoding").get<std::string>();
      response_def.schema = response_json.at("schema").get<std::string>();
      service.response = std::move(response_def);
    }
    return service;
  } catch (const nlohmann::json::exception&) {
    return std::nullopt;
  }
}

auto parseWsAdvertisements(const nlohmann::json& server_txt_msg, WsAdvertisements& ws_server_ads) -> bool {
  try {
    if (!server_txt_msg.contains("op")) {
      return false;
    }

    const std::string op_code = server_txt_msg.at("op").get<std::string>();

    if (op_code == "serverInfo") {
      auto server_info = convertWsJsonMsgtoServerOptions(server_txt_msg);
      if (server_info) {
        ws_server_ads.info = *server_info;
      } else {
        return false;
      }
    } else if (op_code == "advertise") {
      for (const auto& channel_json : server_txt_msg["channels"]) {
        auto channel_ad = convertWsJsonMsgToChannel(channel_json);
        if (!channel_ad) {
          heph::log(heph::ERROR,
                    fmt::format("Failed to parse channel advertisement: {}", channel_json.dump()));
          continue;
        }

        if (saveSchemaToDatabase(*channel_ad, ws_server_ads.schema_db)) {
          ws_server_ads.channels[channel_ad->id] = *channel_ad;
        } else {
          heph::log(heph::ERROR, "Failed to save schema to database for channel.", "channel_id",
                    channel_ad->id, "topic", channel_ad->topic, "encoding", channel_ad->encoding);
          continue;
        }
      }
    } else if (op_code == "advertiseServices") {
      for (const auto& service_json : server_txt_msg["services"]) {
        auto service_ad = convertWsJsonMsgToService(service_json);
        if (!service_ad) {
          heph::log(heph::ERROR,
                    fmt::format("Failed to parse service advertisement: {}", service_json.dump()));
          continue;
        }

        if (saveSchemaToDatabase(*service_ad, ws_server_ads.schema_db)) {
          ws_server_ads.services[service_ad->id] = *service_ad;
        } else {
          heph::log(heph::WARN, "Failed to save service schemas to database.", "service_id", service_ad->id,
                    "service_name", service_ad->name, "service_type", service_ad->type);
          continue;
        }
      }
    } else {
      return false;  // Unknown op code
    }
    return true;
  } catch (const std::exception& e) {
    heph::log(heph::ERROR, fmt::format("JSON parsing error: {}", e.what()));
    return false;
  }
}

auto parseWsServiceFailure(const nlohmann::json& server_txt_msg, WsServiceFailure& service_failure) -> bool {
  try {
    if (!server_txt_msg.contains("op") ||
        server_txt_msg.at("op").get<std::string>() != "serviceCallFailure") {
      return false;
    }

    if (server_txt_msg.contains("callId") && server_txt_msg.contains("message")) {
      service_failure.call_id = server_txt_msg.at("callId").get<uint32_t>();
      service_failure.error_message = server_txt_msg.at("message").get<std::string>();
      return true;
    }
    return false;
  } catch (const std::exception& e) {
    heph::log(heph::ERROR, fmt::format("JSON parsing error: {}", e.what()));
    return false;
  }
}

}  // namespace heph::ws
