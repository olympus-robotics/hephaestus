#pragma once

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <map>
#include <thread>

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

namespace heph::ws_bridge {

struct ServiceCallState {
  enum class Status { DISPATCHED, FAILED, SUCCESS };

  uint32_t call_id;
  Status status;
  std::chrono::steady_clock::time_point dispatch_time;
  std::chrono::steady_clock::time_point response_time;

  std::optional<WsServerServiceResponse> response;
  std::string error_message;

  explicit ServiceCallState(uint32_t call_id);

  std::optional<std::unique_ptr<google::protobuf::Message>>
  receiveResponse(const WsServerServiceResponse _response, WsServerAdvertisements& ws_server_ads);

  void receiveFailureResponse(const std::string& error_msg);

  bool hasResponse() const;
  bool wasSuccessful() const;
  bool hasFailed() const;

  std::optional<std::chrono::milliseconds> getDurationMs() const;
};

using ServiceCallStateMap = std::map<uint32_t, ServiceCallState>;

bool allServiceCallsFinished(const ServiceCallStateMap& state);

std::string horizontalLine(uint32_t cell_content_width, uint32_t columns);

void printServiceCallStateMap(ServiceCallStateMap& state);

void printAdvertisedServices(const WsServerAdvertisements& ws_server_ads);

void printAdvertisedTopics(const WsServerAdvertisements& ws_server_ads);

}  // namespace heph::ws_bridge
