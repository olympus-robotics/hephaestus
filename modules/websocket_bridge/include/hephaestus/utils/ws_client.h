#pragma once

#include <chrono>
#include <csignal>
#include <cstdint>
#include <map>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <foxglove/websocket/websocket_client.hpp>
#include <google/protobuf/util/json_util.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/utils/protobuf_serdes.h>
#include <hephaestus/utils/signal_handler.h>
#include <hephaestus/utils/stack_trace.h>
#include <hephaestus/utils/ws_protocol.h>
#include <nlohmann/json.hpp>

namespace heph::ws {

using WsClient = foxglove::Client<foxglove::WebSocketNoTls>;

struct ServiceCallState {
  enum class Status : std::uint8_t { SUCCESS = 0, DISPATCHED = 1, FAILED = 2 };

  uint32_t call_id;
  Status status;
  std::chrono::steady_clock::time_point dispatch_time;
  std::chrono::steady_clock::time_point response_time;

  std::optional<WsServerServiceResponse> response;
  std::string error_message;

  explicit ServiceCallState(uint32_t call_id);

  auto receiveResponse(const WsServerServiceResponse& service_response, WsServerAdvertisements& ws_server_ads)
      -> std::optional<std::unique_ptr<google::protobuf::Message>>;

  void receiveFailureResponse(const std::string& error_msg);

  [[nodiscard]] auto hasResponse() const -> bool;
  [[nodiscard]] auto wasSuccessful() const -> bool;
  [[nodiscard]] auto hasFailed() const -> bool;

  [[nodiscard]] auto getDurationMs() const -> std::optional<std::chrono::milliseconds>;
};

using ServiceCallStateMap = std::map<uint32_t, ServiceCallState>;

[[nodiscard]] bool allServiceCallsFinished(const ServiceCallStateMap& state);

[[nodiscard]] std::string horizontalLine(uint32_t cell_content_width, uint32_t columns);

void printServiceCallStateMap(ServiceCallStateMap& state);

void printAdvertisedServices(const WsServerAdvertisements& ws_server_ads);

void printAdvertisedTopics(const WsServerAdvertisements& ws_server_ads);

void printClientChannelAds(const std::vector<WsServerClientChannelAd>& client_ads);

}  // namespace heph::ws
