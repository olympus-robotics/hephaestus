#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <thread>
#include <unordered_map>

#include <fmt/core.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <foxglove/websocket/websocket_client.hpp>
#include <foxglove/websocket/websocket_notls.hpp>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <hephaestus/websocket_bridge/protobuf_utils.h>
#include <hephaestus/websocket_bridge/serialization.h>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

static std::atomic<bool> g_abort{ false };

void signalHandler(int) {
  g_abort = true;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fmt::print("Usage: {} <url>\n", argv[0]);
    return 1;
  }

  const std::string url = argv[1];
  foxglove::Client<foxglove::WebSocketNoTls> client;

  // Collect advertisement data
  std::unordered_map<foxglove::ChannelId, foxglove::Channel> channels;
  std::unordered_map<foxglove::ServiceId, foxglove::Service> services;

  // Binary handler for receiving data
  client.setBinaryMessageHandler([&](const uint8_t* data, size_t length) {
    (void)data;
    if (length < 1)
      return;
    fmt::print("Received binary message of length: {}\n", length);
  });

  client.setTextMessageHandler([&](const std::string& jsonMsg) {
    try {
      auto msg = nlohmann::json::parse(jsonMsg);

      // Print the message to a file in the /tmp folder
      std::string fileName = "/tmp/received_message_" + msg["op"].get<std::string>() + ".json";
      std::ofstream outFile(fileName);
      if (outFile.is_open()) {
        outFile << msg.dump(4);  // Pretty print with 4 spaces indentation
        outFile.close();
        fmt::print("Message written to /tmp/received_message.json\n");
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
  });

  // Connect
  auto onOpen = [&](websocketpp::connection_hdl) { fmt::print("Connected to {}\n", url); };
  auto onClose = [&](websocketpp::connection_hdl) {
    fmt::print("Connection closed\n");
    g_abort = true;
  };

  std::signal(SIGINT, signalHandler);
  fmt::print("Connecting to {}...\n", url);
  client.connect(url, onOpen, onClose);

  // Wait for services to be advertised
  fmt::print("Waiting for services to be advertised...\n");
  while (services.empty() && !g_abort) {
    std::this_thread::sleep_for(100ms);
  }

  if (g_abort) {
    fmt::print("Aborted by user.\n");
    client.close();
    return 0;
  }

  // Pick the first advertised service as target
  const auto foxglove_service_pair = services.begin();
  const auto foxglove_service_id = foxglove_service_pair->first;
  const auto& foxglove_service = foxglove_service_pair->second;

  if (!foxglove_service.request.has_value()) {
    fmt::print("Service request definition is missing.\n");
    g_abort = true;
    return 1;
  }

  fmt::print("\nTargeting Service '{}' for stress testing\n", foxglove_service.name);

  fmt::print("Service Details:\n");
  fmt::print("  ID: {}\n", foxglove_service.id);
  fmt::print("  Name: {}\n", foxglove_service.name);
  fmt::print("  Type: {}\n", foxglove_service.type);

  if (foxglove_service.request.has_value()) {
    fmt::print("  Request:\n");
    fmt::print("    Encoding: {}\n", foxglove_service.request->encoding);
    fmt::print("    Schema Name: {}\n", foxglove_service.request->schemaName);
    fmt::print("    Schema Encoding: {}\n", foxglove_service.request->schemaEncoding);
    fmt::print("    Schema: {}\n", foxglove_service.request->schema);
  } else {
    fmt::print("  Request: Not available\n");
  }

  if (foxglove_service.response.has_value()) {
    fmt::print("  Response:\n");
    fmt::print("    Encoding: {}\n", foxglove_service.response->encoding);
    fmt::print("    Schema Name: {}\n", foxglove_service.response->schemaName);
    fmt::print("    Schema Encoding: {}\n", foxglove_service.response->schemaEncoding);
    fmt::print("    Schema: {}\n", foxglove_service.response->schema);
  } else {
    fmt::print("  Response: Not available\n");
  }

  // Example stress: repeatedly call a service
  const auto startTime = std::chrono::steady_clock::now();
  std::vector<std::chrono::milliseconds> callDurations;

    for (int i = 0; i < 100 && !g_abort; ++i) {
    foxglove::ServiceRequest request;
    request.callId = static_cast<uint32_t>(i + 1);  // Increment or generate unique call ID as needed
    request.serviceId = foxglove_service_id;

    // Generate a random protobuf message based on the service definition
    auto message = heph::ws_bridge::generateRandomProtobufMessageFromSchema(foxglove_service.request.value());

    if (message.empty()) {
      fmt::print("Failed to generate random protobuf message for service '{}'\n", foxglove_service.name);
      g_abort = true;
      break;
    }

    request.data.assign(message.begin(), message.end());
    request.encoding = "protobuf";

    fmt::print("Sending service request {}...\n", request.callId);
    const auto callStartTime = std::chrono::steady_clock::now();
    client.sendServiceRequest(request);
    const auto callEndTime = std::chrono::steady_clock::now();
    callDurations.push_back(
        std::chrono::duration_cast<std::chrono::milliseconds>(callEndTime - callStartTime));
    fmt::print("Service request {} sent.\n", request.callId);
  }

  fmt::print("Closing client...\n");
  client.close();
  const auto endTime = std::chrono::steady_clock::now();
  const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

  fmt::print("Test duration: {} ms\n", durationMs);
  for (size_t i = 0; i < callDurations.size(); ++i) {
    fmt::print("Call {} duration: {} ms\n", i + 1, callDurations[i].count());
  }
  fmt::print("Done.\n");
  return 0;
}
