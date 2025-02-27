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
#include <hephaestus/utils/signal_handler.h>
#include <hephaestus/utils/stack_trace.h>
#include <hephaestus/websocket_bridge/protobuf_utils.h>
#include <hephaestus/websocket_bridge/serialization.h>
#include <hephaestus/websocket_bridge/ws_server_utils.h>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

constexpr int SERVICE_REQUEST_COUNT = 10;
constexpr int SPINNING_SLEEP_DURATION_MS = 1000;
constexpr int LAUNCHING_SLEEP_DURATION_MS = 0;
constexpr int RESPONSE_WAIT_DURATION_S = 1;

static std::atomic<bool> g_abort{ false };

void sigintHandler(int signal) {
  fmt::println("Received signal: {}", signal);
  g_abort = true;
}

struct ServiceCallState {
  enum class Status { DISPATCHED, FAILED, SUCCESS };

  uint32_t call_id;
  Status status;
  std::chrono::steady_clock::time_point dispatch_time;
  std::chrono::steady_clock::time_point response_time;

  std::optional<heph::ws_bridge::WsServerServiceResponse> response;
  std::string error_message;

  explicit ServiceCallState(uint32_t call_id)
    : call_id(call_id), status(Status::DISPATCHED), dispatch_time(std::chrono::steady_clock::now()) {
  }

  std::optional<std::unique_ptr<google::protobuf::Message>>
  receiveResponse(const heph::ws_bridge::WsServerServiceResponse _response,
                  heph::ws_bridge::WsServerAdvertisements& ws_server_ads) {
    if (_response.callId != call_id) {
      heph::log(heph::ERROR, "Mismatched call ID", "expected_call_id", call_id, "received_call_id",
                _response.callId);
      return std::nullopt;
    }

    if (_response.encoding != "protobuf") {
      heph::log(heph::ERROR, "Unexpected encoding in service response", "expected", "protobuf", "received",
                _response.encoding);
      status = Status::FAILED;
      return std::nullopt;
    }

    auto message =
        heph::ws_bridge::retrieveResponseMessageFromDatabase(_response.serviceId, ws_server_ads.schema_db);
    if (!message) {
      heph::log(heph::ERROR, "Failed to response retrieve message from database", "call_id", call_id,
                "service_id", _response.serviceId);
      status = Status::FAILED;
      return std::nullopt;
    }

    if (!message->ParseFromArray(_response.data.data(), static_cast<int>(_response.data.size()))) {
      heph::log(heph::ERROR, "Failed to parse response data with proto schema", "call_id", call_id,
                "data_size", _response.data.size(), "schema_name", message->GetDescriptor()->full_name());

      status = Status::FAILED;
      return std::nullopt;
    }

    response = _response;
    response_time = std::chrono::steady_clock::now();
    status = Status::SUCCESS;

    return message;
  }

  void receiveFailureResponse(const std::string& error_msg) {
    response_time = std::chrono::steady_clock::now();
    error_message = error_msg;
    status = Status::FAILED;
  }

  bool hasResponse() const {
    const bool has_response = (status == Status::SUCCESS || status == Status::FAILED);

    if (has_response && (!response.has_value() && error_message.empty())) {
      heph::log(heph::ERROR, "Service call has terminated, but neither has a response nor an error msg.",
                "call_id", call_id);
      return false;
    }

    return has_response;
  }

  bool wasSuccessful() const {
    return status == Status::SUCCESS;
  }

  bool hasFailed() const {
    return status == Status::FAILED;
  }

  std::optional<std::chrono::milliseconds> getDurationMs() const {
    if (!hasResponse()) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                   dispatch_time);
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(response_time - dispatch_time);
  }
};

using ServiceCallStateMap = std::map<uint32_t, ServiceCallState>;

bool allServiceCallsFinished(const ServiceCallStateMap& state) {
  for (const auto& [call_id, service_call_state] : state) {
    if (!service_call_state.hasResponse()) {
      return false;
    }
  }
  return true;
}

std::string horizontalLine(uint32_t cell_content_width, uint32_t columns) {
  std::stringstream ss;
  ss << "+";
  for (uint32_t col = 0; col < columns; ++col) {
    for (uint32_t i = 0; i < cell_content_width; ++i) {
      ss << "-";
    }
    ss << "+";
  }
  ss << "\n";
  return ss.str();
}

void printServiceCallStateMap(ServiceCallStateMap& state) {
  constexpr uint32_t max_columns = 5;
  constexpr uint32_t cell_content_width = 17;

  const uint32_t num_service_calls = static_cast<uint32_t>(state.size());
  const uint32_t width = std::min(num_service_calls, max_columns);
  const uint32_t height = (num_service_calls + width - 1) / width;

  fmt::println("Service Call States");

  auto horizontal_line = horizontalLine(cell_content_width, width);
  fmt::print("{}", horizontal_line);

  auto it = state.begin();

  for (uint32_t row_idx = 0; row_idx < height && it != state.end(); ++row_idx) {
    fmt::print("|");
    for (uint32_t col_idx = 0; col_idx < width; ++col_idx) {
      // If we already reached the end of the map, print empty cell.
      if (it == state.end()) {
        fmt::print("{:<{}}|", " ", cell_content_width);
        continue;
      }

      // Print the valid cell content.
      const auto& service_call_state = it->second;
      const uint32_t call_id = service_call_state.call_id;

      const std::string status_str =
          service_call_state.hasResponse() ? (service_call_state.wasSuccessful() ? "✔" : "✖") : "∅";

      fmt::print("{:<{}}|",
                 fmt::format(" {:03}  {:1}  {:4}ms ", call_id, status_str,
                             service_call_state.getDurationMs().value().count()),
                 cell_content_width);

      ++it;
    }
    fmt::print("\n{}", horizontal_line);
  }
}

void handleBinaryMessage(const uint8_t* data, size_t length,
                         heph::ws_bridge::WsServerAdvertisements& ws_server_ads, ServiceCallStateMap& state) {
  if (data == nullptr || length == 0) {
    fmt::print("Received invalid message.\n");
    return;
  }

  const uint8_t opcode = data[0];

  if (opcode == static_cast<uint8_t>(foxglove::BinaryOpcode::SERVICE_CALL_RESPONSE)) {
    // Skipping the first byte (opcode)
    const uint8_t* payload = data + 1;
    const size_t payload_size = length - 1;

    foxglove::ServiceResponse response;
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

    // DEBUG PRINT ONLY
    if (msg.has_value()) {
      std::string json_string;
      const auto status = google::protobuf::util::MessageToJsonString(**msg, &json_string);
      if (!status.ok()) {
        fmt::print("Failed to convert message to JSON: {}\n", status.ToString());
        return;
      }
      fmt::print("Service response for call ID {}:\n'''\n{}\n'''\n", response.callId, json_string);
    }
    return;
  }
}

void handleJsonMessage(const std::string& json_msg, heph::ws_bridge::WsServerAdvertisements& ws_server_ads,
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
  if (heph::ws_bridge::parseWsServerAdvertisements(msg, ws_server_ads)) {
    // Everything is alright.
    return;
  }

  // Handle Service Failures
  heph::ws_bridge::WsServerServiceFailure service_failure;
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

void printAdvertisedServices(const heph::ws_bridge::WsServerAdvertisements& ws_server_ads) {
  fmt::println("Advertised services:");
  fmt::println("--------------------------------------------------");
  if (ws_server_ads.services.empty()) {
    fmt::println("No services advertised.");
    fmt::println("--------------------------------------------------");
    return;
  }
  for (const auto& [service_id, service] : ws_server_ads.services) {
    fmt::println("Service ID   : {}", service_id);
    fmt::println("Name         : {}", service.name);
    fmt::println("Type         : {}", service.type);
    if (service.request.has_value()) {
      fmt::println("Request:");
      fmt::println("  Encoding      : {}", service.request->encoding);
      fmt::println("  Schema Name   : {}", service.request->schemaName);
      fmt::println("  Schema Enc.   : {}", service.request->schemaEncoding);
    } else {
      fmt::println("Request      : None");
    }
    if (service.response.has_value()) {
      fmt::println("Response:");
      fmt::println("  Encoding      : {}", service.response->encoding);
      fmt::println("  Schema Name   : {}", service.response->schemaName);
      fmt::println("  Schema Enc.   : {}", service.response->schemaEncoding);
    } else {
      fmt::println("Response     : None");
    }
    fmt::println("--------------------------------------------------");
  }
}

void sendTestServiceRequests(foxglove::Client<foxglove::WebSocketNoTls>& client,
                             const foxglove::Service& foxglove_service,
                             heph::ws_bridge::WsServerAdvertisements& ws_server_ads,
                             ServiceCallStateMap& state) {
  auto foxglove_service_id = foxglove_service.id;

  for (int i = 1; i <= SERVICE_REQUEST_COUNT && !g_abort; ++i) {
    foxglove::ServiceRequest request;
    request.callId = static_cast<uint32_t>(i);
    request.serviceId = foxglove_service_id;

    auto message = heph::ws_bridge::generateRandomMessageFromSchemaName(foxglove_service.request->schemaName,
                                                                        ws_server_ads.schema_db);
    if (!message) {
      fmt::println("Failed to generate random protobuf message for service '{}'", foxglove_service.name);
      g_abort = true;
      break;
    }

    // DEBUG PRINT ONLY
    {
      std::string json_string;
      const auto status = google::protobuf::util::MessageToJsonString(*message, &json_string);
      if (!status.ok()) {
        fmt::println("Failed to convert request message to JSON: {}", status.ToString());
        break;
      }
      fmt::println("Sending service request with call ID {}:\n'''\n{}\n'''", request.callId, json_string);
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
  foxglove::Client<foxglove::WebSocketNoTls> client;

  heph::ws_bridge::WsServerAdvertisements ws_server_ads;
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