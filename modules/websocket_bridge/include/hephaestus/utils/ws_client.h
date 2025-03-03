#pragma once

#include <chrono>
#include <csignal>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <foxglove/websocket/websocket_client.hpp>
#include <foxglove/websocket/websocket_notls.hpp>
#include <google/protobuf/message.h>
#include <hephaestus/utils/ws_protocol.h>

namespace heph::ws {

using WsClientNoTls = foxglove::Client<foxglove::WebSocketNoTls>;

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

[[nodiscard]] auto allServiceCallsFinished(const ServiceCallStateMap& state) -> bool;

[[nodiscard]] auto horizontalLine(uint32_t cell_content_width, uint32_t columns) -> std::string;

void printServiceCallStateMap(ServiceCallStateMap& state);

void printAdvertisedServices(const WsServerAdvertisements& ws_server_ads);

void printAdvertisedTopics(const WsServerAdvertisements& ws_server_ads);

void printClientChannelAds(const std::vector<WsServerClientChannelAd>& client_ads);

}  // namespace heph::ws
