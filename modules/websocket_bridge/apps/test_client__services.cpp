#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <thread>
#include <unordered_map>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <foxglove/websocket/websocket_client.hpp>
#include <google/protobuf/util/json_util.h>
#include <hephaestus/websocket_bridge/protobuf_test_helpers.h>
#include <hephaestus/websocket_bridge/serialization.h>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

constexpr size_t MIN_MESSAGE_LENGTH = 12u;
constexpr int SERVICE_REQUEST_COUNT = 100;
constexpr int SLEEP_DURATION_MS = 100;
constexpr int RESPONSE_WAIT_DURATION_S = 1;

static std::atomic<bool> g_abort{ false };

void sigintHandler(int signal) {
  fmt::print("Received signal: {}\n", signal);
  g_abort = true;
}

using ResponsesWithTimingMap =
    std::unordered_map<uint32_t, std::pair<foxglove::ServiceResponse, std::chrono::milliseconds>>;

void printResultTable(const ResponsesWithTimingMap& responses, uint32_t A, uint32_t B) {
  constexpr uint32_t max_columns = 5;
  uint32_t range = B - A + 1;
  uint32_t width = std::min(range, max_columns);
  uint32_t height = (range + width - 1) / width;

  fmt::print("Checking presence of keys from {} to {}:\n", A, B);
  fmt::print("+");
  for (uint32_t i = 0; i < width; ++i) {
    fmt::print("---------------+");
  }
  fmt::print("\n");

  for (uint32_t row = 0; row < height; ++row) {
    fmt::print("|");
    for (uint32_t col = 0; col < width; ++col) {
      uint32_t value = A + row * width + col;
      if (value > B) {
        fmt::print("       |");
      } else {
        if (responses.count(value)) {
          fmt::print(" {:4} ✔ {:4}ms |", value, responses.at(value).second.count());
        } else {
          fmt::print(" {:4} ∅      |", value);
        }
      }
    }
    fmt::print("\n+");
    for (uint32_t i = 0; i < width; ++i) {
      fmt::print("---------------+");
    }
    fmt::print("\n");
  }
}

void handleBinaryMessage(
    const uint8_t* data, size_t length,
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point>& call_id_to_start_time,
    ResponsesWithTimingMap& responses) {
  if (data == nullptr || length == 0) {
    fmt::print("Received invalid message.\n");
    return;
  }

  if (length < MIN_MESSAGE_LENGTH) {
    fmt::print("Received message with length {} is too short. (min {} bytes)\n", length, MIN_MESSAGE_LENGTH);
    return;
  }

  if (data[0] != static_cast<uint8_t>(foxglove::BinaryOpcode::SERVICE_CALL_RESPONSE)) {
    fmt::print("Received message with opcode {} is not a service call response.\n", data[0]);
    return;
  }

  const uint8_t opcode = data[0];
  fmt::print("Received opcode: {}\n", opcode);

  const uint8_t* payload = data + 1;
  const size_t payload_size = length - 1;

  foxglove::ServiceResponse response;
  try {
    response.read(payload, payload_size);
  } catch (const std::exception& e) {
    fmt::print("Failed to deserialize service response: {}\n", e.what());
    return;
  }

  fmt::print("Service Response:\n");
  fmt::print("  Service ID: {}\n", response.serviceId);
  fmt::print("  Call ID: {}\n", response.callId);
  fmt::print("  Encoding: {}\n", response.encoding);
  fmt::print("  Data (Base64): '{}'\n",
             foxglove::base64Encode(std::string_view(reinterpret_cast<const char*>(response.data.data()),
                                                     response.data.size())));

  auto it = call_id_to_start_time.find(response.callId);
  if (it != call_id_to_start_time.end()) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - it->second);
    fmt::print("Service call {} took {} ms\n", response.callId, duration);

    responses[response.callId] = std::make_pair(response, duration);
  } else {
    fmt::print("Failed to measure response time for call ID {}.\n", response.callId);
  }
}

void handleTextMessage(const std::string& jsonMsg, std::map<foxglove::ChannelId, foxglove::Channel>& channels,
                       std::map<foxglove::ServiceId, foxglove::Service>& services) {
  try {
    auto msg = nlohmann::json::parse(jsonMsg);

    std::string fileName = "/tmp/received_message_" + msg["op"].get<std::string>() + ".json";
    std::ofstream outFile(fileName);
    if (outFile.is_open()) {
      outFile << msg.dump(4);
      outFile.close();
      fmt::print("Message written to {}\n", fileName);
    } else {
      fmt::print("Failed to open file for writing\n");
    }

    if (msg.contains("op")) {
      const auto& op = msg["op"].get<std::string>();
      if (op == "serverInfo") {
        fmt::print("Server Info: {}\n", msg.dump());
      } else if (op == "advertise") {
        for (const auto& channel : msg["channels"]) {
          foxglove::Channel ch;
          ch.id = channel["id"];
          ch.topic = channel["topic"];
          ch.encoding = channel["encoding"];
          ch.schemaName = channel["schemaName"];
          if (channel.contains("schema")) {
            ch.schema = channel["schema"];
          }
          if (channel.contains("schemaEncoding")) {
            ch.schemaEncoding = channel["schemaEncoding"];
          }
          channels[ch.id] = ch;
          fmt::print("Advertised channel: {}\n", ch.topic);
        }
      } else if (op == "advertiseServices") {
        for (const auto& service : msg["services"]) {
          foxglove::Service foxglove_service;
          foxglove_service.id = service["id"];
          foxglove_service.name = service["name"];
          foxglove_service.type = service["type"];
          foxglove_service.request = foxglove::ServiceRequestDefinition{
            .encoding = service["request"]["encoding"],
            .schemaName = service["request"]["schemaName"],
            .schemaEncoding = service["request"]["schemaEncoding"],
            .schema = service["request"]["schema"],
          };
          foxglove_service.response = foxglove::ServiceResponseDefinition{
            .encoding = service["response"]["encoding"],
            .schemaName = service["response"]["schemaName"],
            .schemaEncoding = service["response"]["schemaEncoding"],
            .schema = service["response"]["schema"],
          };
          fmt::print("Advertised service: {}\n", foxglove_service.name);
          services[foxglove_service.id] = foxglove_service;
        }
      } else {
        fmt::print("Unknown operation: {}\n", op);
        fmt::print("Raw Message: {}\n", jsonMsg);
        g_abort = true;
      }
    }
  } catch (const nlohmann::json::parse_error& e) {
    fmt::print("JSON parse error: {}\n", e.what());
    g_abort = true;
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fmt::print("Usage: {} <url>\n", argv[0]);
    return 1;
  }

  const std::string url = argv[1];
  foxglove::Client<foxglove::WebSocketNoTls> client;

  std::map<foxglove::ChannelId, foxglove::Channel> channels;
  std::map<foxglove::ServiceId, foxglove::Service> services;
  ResponsesWithTimingMap responses;
  std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> call_id_to_start_time;

  auto binary_message_handler = [&](const uint8_t* data, size_t length) {
    handleBinaryMessage(data, length, call_id_to_start_time, responses);
  };
  auto on_open_handler = [&](websocketpp::connection_hdl) { fmt::print("Connected to {}\n", url); };
  auto on_close_handler = [&](websocketpp::connection_hdl) {
    fmt::print("Connection closed\n");
    g_abort = true;
  };
  auto text_message_handler = [&](const std::string& jsonMsg) {
    handleTextMessage(jsonMsg, channels, services);
  };

  std::signal(SIGINT, sigintHandler);

  client.setBinaryMessageHandler(binary_message_handler);
  client.setTextMessageHandler(text_message_handler);

  fmt::print("Connecting to {}...\n", url);
  client.connect(url, on_open_handler, on_close_handler);

  fmt::print("Waiting for services to be advertised...\n");
  while (services.empty() && !g_abort) {
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_MS));
  }

  fmt::print("Advertised services:\n");
  for (const auto& [serviceId, service] : services) {
    fmt::print("Service ID: {}, Name: {}, Type: {}\n", serviceId, service.name, service.type);
    if (service.request.has_value()) {
      fmt::print("  Request:\n");
      fmt::print("    Encoding: {}\n", service.request->encoding);
      fmt::print("    Schema Name: {}\n", service.request->schemaName);
      fmt::print("    Schema Encoding: {}\n", service.request->schemaEncoding);
      fmt::print("    Schema: {}\n", service.request->schema);
    } else {
      fmt::print("  Request: None\n");
    }
    if (service.response.has_value()) {
      fmt::print("  Response:\n");
      fmt::print("    Encoding: {}\n", service.response->encoding);
      fmt::print("    Schema Name: {}\n", service.response->schemaName);
      fmt::print("    Schema Encoding: {}\n", service.response->schemaEncoding);
      fmt::print("    Schema: {}\n", service.response->schema);
    } else {
      fmt::print("  Response: None\n");
    }
  }

  auto foxglove_service_pair = std::find_if(services.begin(), services.end(), [](const auto& pair) {
    return !pair.second.name.starts_with("topic_info");
  });

  if (foxglove_service_pair == services.end()) {
    fmt::print("No suitable service found.\n");
    g_abort = true;
    return 1;
  }

  const auto foxglove_service_id = foxglove_service_pair->first;
  const auto& foxglove_service = foxglove_service_pair->second;

  if (!foxglove_service.request.has_value()) {
    fmt::print("Service request definition is missing.\n");
    g_abort = true;
    return 1;
  }

  fmt::print("\nTargeting Service '{}' testing\n", foxglove_service.name);

  for (int i = 0; i < SERVICE_REQUEST_COUNT && !g_abort; ++i) {
    foxglove::ServiceRequest request;
    request.callId = static_cast<uint32_t>(i) + 1;
    request.serviceId = foxglove_service_id;

    auto message = heph::ws_bridge::generateRandomProtobufMessageFromSchema(foxglove_service.request.value());

    if (message.empty()) {
      fmt::print("Failed to generate random protobuf message for service '{}'\n", foxglove_service.name);
      g_abort = true;
      break;
    }

    request.data.assign(message.begin(), message.end());
    request.encoding = "protobuf";

    call_id_to_start_time[request.callId] = std::chrono::steady_clock::now();
    client.sendServiceRequest(request);
  }

  while (responses.size() < SERVICE_REQUEST_COUNT && !g_abort) {
    fmt::print("Waiting for responses... [Ctrl-C to abort]\n");
    std::this_thread::sleep_for(std::chrono::seconds(RESPONSE_WAIT_DURATION_S));
    printResultTable(responses, 1, SERVICE_REQUEST_COUNT);
  }

  fmt::print("Closing client...\n");
  client.close();
  fmt::print("Done.\n");
  return 0;
}
