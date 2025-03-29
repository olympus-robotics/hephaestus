#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <fmt/base.h>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/websocket_client.hpp>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/utils/stack_trace.h>
#include <hephaestus/websocket_bridge/utils/protobuf_serdes.h>
#include <hephaestus/websocket_bridge/utils/ws_client.h>
#include <hephaestus/websocket_bridge/utils/ws_protocol.h>
#include <nlohmann/json_fwd.hpp>

using namespace std::chrono_literals;
using heph::ws::ServiceCallState;
using heph::ws::ServiceCallStateMap;
using heph::ws::WsAdvertisements;
using heph::ws::WsBinaryOpCode;
using heph::ws::WsClientHandle;
using heph::ws::WsClientNoTls;
using heph::ws::WsServiceAd;
using heph::ws::WsServiceFailure;
using heph::ws::WsServiceRequest;
using heph::ws::WsServiceResponse;

constexpr int SERVICE_REQUEST_COUNT = 20;
constexpr int SPINNING_SLEEP_DURATION_MS = 1000;
constexpr int LAUNCHING_SLEEP_DURATION_MS = 0;
constexpr int RESPONSE_WAIT_DURATION_S = 1;
constexpr int SERVCE_CALL_TIMEOUT_MS = 1000;
namespace {

std::atomic<bool> g_abort{ false };  // NOLINT

void sigintHandler(int signal) {
  fmt::println("Received signal: {}", signal);
  g_abort = true;
}

void handleBinaryMessage(const uint8_t* data, size_t length, WsAdvertisements& ws_server_ads,
                         ServiceCallStateMap& state) {
  if (data == nullptr || length == 0) {
    fmt::println("Received invalid message.");
    return;
  }

  const uint8_t opcode = data[0];  // NOLINT

  if (opcode == static_cast<uint8_t>(WsBinaryOpCode::SERVICE_CALL_RESPONSE)) {
    // Skipping the first byte (opcode)
    const uint8_t* payload = data + 1;  // NOLINT
    const size_t payload_size = length - 1;

    WsServiceResponse response;
    try {
      response.read(payload, payload_size);
    } catch (const std::exception& e) {
      heph::log(heph::ERROR, "Failed to deserialize service response", "exception", e.what());
      return;
    }

    auto lock = state.scopedLock();

    // Check that we already have dispatched a service call with this ID.
    auto state_it = state.find(response.callId);
    if (state_it == state.end()) {
      heph::log(heph::ERROR, "No record of a service call with this id.", "call_id", response.callId);
      return;
    }

    // Receive, parse and convert response to Protobuf message.
    auto msg = receiveResponse(response, ws_server_ads, state_it->second);
    return;
  }
}

void handleJsonMessage(const std::string& json_msg, WsAdvertisements& ws_server_ads,
                       ServiceCallStateMap& state) {
  // Parse the JSON message
  nlohmann::json msg;
  try {
    msg = nlohmann::json::parse(json_msg);
  } catch (const std::exception& e) {
    heph::log(heph::ERROR, "JSON parse error.", "json_msg", json_msg, "exception", e.what());
    g_abort = true;
    return;
  }

  // Handle Advertisements
  if (parseWsAdvertisements(msg, ws_server_ads)) {
    // Everything is alright.
    return;
  }

  // Handle Service Failures
  WsServiceFailure service_failure;
  if (parseWsServiceFailure(msg, service_failure)) {
    heph::log(heph::ERROR, "Service call failed with error.", "call_id", service_failure.call_id,
              "error_message", service_failure.error_message);

    auto lock = state.scopedLock();

    auto state_it = state.find(service_failure.call_id);
    if (state_it == state.end()) {
      heph::log(heph::ERROR, "No record of a service call with this id.", "call_id", service_failure.call_id);
      return;
    }

    receiveFailureResponse(service_failure.error_message, state_it->second);
  }
}

void sendTestServiceRequests(WsClientNoTls& client, const WsServiceAd& foxglove_service,
                             WsAdvertisements& ws_server_ads, ServiceCallStateMap& state) {
  auto foxglove_service_id = foxglove_service.id;

  for (int i = 1; i <= SERVICE_REQUEST_COUNT && !g_abort; ++i) {
    WsServiceRequest request;
    request.callId = static_cast<uint32_t>(i);
    request.serviceId = foxglove_service_id;
    request.timeoutMs = SERVCE_CALL_TIMEOUT_MS;

    if (!foxglove_service.request.has_value()) {
      fmt::println("Service '{}' has no request definition", foxglove_service.name);
      g_abort = true;
      break;
    }

    auto message = generateRandomMessageFromSchemaName(foxglove_service.request.value().schemaName,
                                                       ws_server_ads.schema_db);
    if (message == nullptr) {
      fmt::println("Failed to generate random protobuf message for service '{}'", foxglove_service.name);
      g_abort = true;
      break;
    }

    // Prepare message for sending.
    std::vector<uint8_t> message_buffer(message->ByteSizeLong());
    if (!message->SerializeToArray(message_buffer.data(), static_cast<int>(message_buffer.size()))) {
      fmt::println("Failed to serialize message");
      break;
    }
    if (message_buffer.empty()) {
      fmt::println("Failed to generate random protobuf message for service '{}'", foxglove_service.name);
      g_abort = true;
      break;
    }
    request.data.assign(message_buffer.begin(), message_buffer.end());
    request.encoding = "protobuf";

    {
      auto lock = state.scopedLock();

      // Init the service call as dispatched.
      state.emplace(request.callId, ServiceCallState(request.callId));
    }

    // Dispatch the service request.
    client.sendServiceRequest(request);

    fmt::println("Service request with call ID {} dispatched", request.callId);

    // Optionally sleep before launching the next request. This can be useful to explore/investigate the
    // performance of the services and how the queue up in the bridge.
    if (LAUNCHING_SLEEP_DURATION_MS > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(LAUNCHING_SLEEP_DURATION_MS));
    }
  }
}

}  // namespace

// NOLINTBEGIN(clang-analyzer-optin.cplusplus.VirtualCall)
// Note: This NOLINT is needed because it is triggered by an issue in the virtual
// destructor of foxglove::Client, i.e. inside the dependency, not our code

auto main(int argc, char** argv) -> int try {
  const heph::utils::StackTrace stack_trace;
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  if (argc < 2) {
    fmt::println("Usage: {} <url> (e.g. ws://localhost:8765)", argv[0]);  // NOLINT
    return 1;
  }

  const std::string url = argv[1];  // NOLINT
  WsClientNoTls client;

  WsAdvertisements ws_server_ads;
  ServiceCallStateMap state;

  const auto binary_message_handler = [&](const uint8_t* data, size_t length) {
    handleBinaryMessage(data, length, ws_server_ads, state);
  };
  // NOLINTNEXTLINE
  const auto on_open_handler = [&](WsClientHandle) { fmt::println("Connected to {}", url); };
  // NOLINTNEXTLINE
  const auto on_close_handler = [&](WsClientHandle) {
    fmt::println("Connection closed");
    g_abort = true;
  };
  const auto json_msg_handler = [&](const std::string& json_msg) {
    handleJsonMessage(json_msg, ws_server_ads, state);
  };

  if (std::signal(SIGINT, sigintHandler) == SIG_ERR) {
    fmt::println("Error setting up signal handler.");
    return 1;
  }

  client.setBinaryMessageHandler(binary_message_handler);
  client.setTextMessageHandler(json_msg_handler);

  fmt::println("Connecting to {}...", url);
  client.connect(url, on_open_handler, on_close_handler);

  fmt::println("Waiting for services to be advertised...");
  while (ws_server_ads.services.empty() && !g_abort) {
    std::this_thread::sleep_for(std::chrono::milliseconds(SPINNING_SLEEP_DURATION_MS));
  }

  printAdvertisedServices(ws_server_ads);

  const auto foxglove_service_pair = std::ranges::find_if(
      ws_server_ads.services, [](const auto& pair) { return !pair.second.name.starts_with("topic_info"); });

  if (foxglove_service_pair == ws_server_ads.services.end()) {
    fmt::println("No suitable service found.");
    g_abort = true;
    return 1;
  }

  const auto& foxglove_service = foxglove_service_pair->second;

  fmt::println("\nTargeting Service '{}' testing", foxglove_service.name);

  sendTestServiceRequests(client, foxglove_service, ws_server_ads, state);

  printServiceCallStateMap(state);

  while (!allServiceCallsFinished(state) && !g_abort) {
    fmt::println("Waiting for responses... [Ctrl-C to abort]");
    std::this_thread::sleep_for(std::chrono::seconds(RESPONSE_WAIT_DURATION_S));

    printServiceCallStateMap(state);
  }

  fmt::println("Closing client...");
  client.close();
  fmt::println("Done.");
  return 0;
} catch (const std::exception& e) {
  fmt::println("Exception caught in main: {}", e.what());
  return 1;
}

// NOLINTEND(clang-analyzer-optin.cplusplus.VirtualCall)
