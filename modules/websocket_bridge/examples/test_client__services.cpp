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
#include <hephaestus/utils/ws_client.h>
#include <hephaestus/utils/ws_protocol.h>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;
using namespace heph::ws;

constexpr int SERVICE_REQUEST_COUNT = 8;
constexpr int SPINNING_SLEEP_DURATION_MS = 1000;
constexpr int LAUNCHING_SLEEP_DURATION_MS = 0;
constexpr int RESPONSE_WAIT_DURATION_S = 1;

static std::atomic<bool> g_abort{ false };

void sigintHandler(int signal) {
  fmt::println("Received signal: {}", signal);
  g_abort = true;
}

void handleBinaryMessage(const uint8_t* data, size_t length, WsServerAdvertisements& ws_server_ads,
                         ServiceCallStateMap& state) {
  if (data == nullptr || length == 0) {
    fmt::print("Received invalid message.\n");
    return;
  }

  const uint8_t opcode = data[0];

  if (opcode == static_cast<uint8_t>(WsServerBinaryOpCode::SERVICE_CALL_RESPONSE)) {
    // Skipping the first byte (opcode)
    const uint8_t* payload = data + 1;
    const size_t payload_size = length - 1;

    WsServerServiceResponse response;
    try {
      response.read(payload, payload_size);
    } catch (const std::exception& e) {
      heph::log(heph::ERROR, "Failed to deserialize service response", "exception", e.what());
      return;
    }

    // Check that we already have dispatched a service call with this ID.
    auto state_it = state.find(response.callId);
    if (state_it == state.end()) {
      heph::log(heph::ERROR, "No record of a service call with this id.", "call_id", response.callId);
      return;
    }

    // Receive, parse and convert response to Protobuf message.
    auto msg = state_it->second.receiveResponse(response, ws_server_ads);

    // TODO(mfehr): REMOVE
    if (msg.has_value()) {
      debugPrintMessage(**msg);
    }
    return;
  }
}

void handleJsonMessage(const std::string& json_msg, WsServerAdvertisements& ws_server_ads,
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
  if (parseWsServerAdvertisements(msg, ws_server_ads)) {
    // Everything is alright.
    return;
  }

  // Handle Service Failures
  WsServerServiceFailure service_failure;
  if (parseWsServerServiceFailure(msg, service_failure)) {
    heph::log(heph::ERROR, "Service call failed with error.", "call_id", service_failure.call_id,
              "error_message", service_failure.error_message);

    auto state_it = state.find(service_failure.call_id);
    if (state_it == state.end()) {
      heph::log(heph::ERROR, "No record of a service call with this id.", "call_id", service_failure.call_id);
      return;
    }

    state_it->second.receiveFailureResponse(service_failure.error_message);
  }
}

void sendTestServiceRequests(WsClient& client,
                             const WsServerServiceAd& foxglove_service, WsServerAdvertisements& ws_server_ads,
                             ServiceCallStateMap& state) {
  auto foxglove_service_id = foxglove_service.id;

  for (int i = 1; i <= SERVICE_REQUEST_COUNT && !g_abort; ++i) {
    WsServerServiceRequest request;
    request.callId = static_cast<uint32_t>(i);
    request.serviceId = foxglove_service_id;

    auto message =
        generateRandomMessageFromSchemaName(foxglove_service.request->schemaName, ws_server_ads.schema_db);
    if (!message) {
      fmt::println("Failed to generate random protobuf message for service '{}'", foxglove_service.name);
      g_abort = true;
      break;
    }

    // TODO(mfehr): REMOVE
    debugPrintMessage(*message);

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

    // Init the service call as dispatched.
    state.emplace(request.callId, ServiceCallState(request.callId));

    // Dispatch the service request.
    client.sendServiceRequest(request);

    fmt::println("Service request with call ID {} dispatched", request.callId);

    // Optionally sleep before launching the next request. This can be useful to explore/investiagate the
    // performance of the services and how the queue up in the bridge.
    if (LAUNCHING_SLEEP_DURATION_MS > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(LAUNCHING_SLEEP_DURATION_MS));
    }
  }
}

int main(int argc, char** argv) {
  const heph::utils::StackTrace stack_trace;
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  if (argc < 2) {
    fmt::println("Usage: {} <url> (e.g. ws://localhost:8765)", argv[0]);
    return 1;
  }

  const std::string url = argv[1];
  WsClient client;

  WsServerAdvertisements ws_server_ads;
  ServiceCallStateMap state;

  const auto binary_message_handler = [&](const uint8_t* data, size_t length) {
    handleBinaryMessage(data, length, ws_server_ads, state);
  };
  const auto on_open_handler = [&](websocketpp::connection_hdl) { fmt::print("Connected to {}\n", url); };
  const auto on_close_handler = [&](websocketpp::connection_hdl) {
    fmt::println("Connection closed");
    g_abort = true;
  };
  const auto json_msg_handler = [&](const std::string& jsonMsg) {
    handleJsonMessage(jsonMsg, ws_server_ads, state);
  };

  std::signal(SIGINT, sigintHandler);

  client.setBinaryMessageHandler(binary_message_handler);
  client.setTextMessageHandler(json_msg_handler);

  fmt::println("Connecting to {}...", url);
  client.connect(url, on_open_handler, on_close_handler);

  fmt::println("Waiting for services to be advertised...");
  while (ws_server_ads.services.empty() && !g_abort) {
    std::this_thread::sleep_for(std::chrono::milliseconds(SPINNING_SLEEP_DURATION_MS));
  }

  printAdvertisedServices(ws_server_ads);

  const auto foxglove_service_pair =
      std::find_if(ws_server_ads.services.begin(), ws_server_ads.services.end(),
                   [](const auto& pair) { return !pair.second.name.starts_with("topic_info"); });

  if (foxglove_service_pair == ws_server_ads.services.end()) {
    fmt::println("No suitable service found.");
    g_abort = true;
    return 1;
  }

  const auto& foxglove_service = foxglove_service_pair->second;

  fmt::println("\nTargeting Service '{}' testing", foxglove_service.name);

  sendTestServiceRequests(client, foxglove_service, ws_server_ads, state);

  while (!allServiceCallsFinished(state) && !g_abort) {
    fmt::println("Waiting for responses... [Ctrl-C to abort]");
    std::this_thread::sleep_for(std::chrono::seconds(RESPONSE_WAIT_DURATION_S));

    printServiceCallStateMap(state);
  }

  // IF we wait between launching the requests (LAUNCHING_SLEEP_DURATION_MS), the above loop will not even
  // execute, so we print here again.
  printServiceCallStateMap(state);

  fmt::println("Closing client...");
  client.close();
  fmt::println("Done.");
  return 0;
}