#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>

// #include <iostream>
// #include <random>
// #include <regex>
// #include <thread>
// #include <unordered_map>

#include <fmt/core.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <foxglove/websocket/websocket_client.hpp>
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

template <typename T>
void printOverviewTable(const std::unordered_map<uint32_t, T>& responses, uint32_t A, uint32_t B) {
  uint32_t range = B - A + 1;
  uint32_t side_length = static_cast<uint32_t>(std::ceil(std::sqrt(range)));

  fmt::print("Kecking presence of keys from {} to {}:\n", A, B);
  fmt::print("+");
  for (uint32_t i = 0; i < side_length; ++i) {
    fmt::print("--------+");
  }
  fmt::print("\n");

  for (uint32_t row = 0; row < side_length; ++row) {
    fmt::print("|");
    for (uint32_t col = 0; col < side_length; ++col) {
      uint32_t value = A + row * side_length + col;
      if (value > B) {
        fmt::print("       |");
      } else {
        fmt::print(" {:4}{} |", value, responses.count(value) ? " ✔" : " ∅");
      }
    }
    fmt::print("\n+");
    for (uint32_t i = 0; i < side_length; ++i) {
      fmt::print("--------+");
    }
    fmt::print("\n");
  }
}

void printBinary(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    fmt::print("No data to print.\n");
    return;
  }

  std::stringstream ss;
  for (size_t i = 0; i < length; ++i) {
    for (int bit = 7; bit >= 0; --bit) {
      ss << ((data[i] >> bit) & 1);
      if (bit == 4) {
        ss << " | ";
      }
    }
    if ((i + 1) % 4 == 0) {
      uint32_t uint32_value = foxglove::ReadUint32LE(data + i - 3);
      ss << " ==> " << uint32_value << "\n";
    } else if (i < length - 1) {
      ss << " || ";
    }
  }
  if (length % 4 != 0) {
    ss << "\n";
  }

  fmt::print("{}", ss.str());
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fmt::print("Usage: {} <url>\n", argv[0]);
    return 1;
  }

  const std::string url = argv[1];
  foxglove::Client<foxglove::WebSocketNoTls> client;

  // Collect advertisement data
  std::map<foxglove::ChannelId, foxglove::Channel> channels;
  std::map<foxglove::ServiceId, foxglove::Service> services;
  std::unordered_map<uint32_t, foxglove::ServiceResponse> responses;
  std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> call_id_to_start_time;

  // Binary handler for receiving data
  client.setBinaryMessageHandler([&](const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
      fmt::print("Received invalid message.\n");
      return;
    }

    if (length < 12u) {
      fmt::print("Received message with length {} is too short. (min 12 bytes)\n", length);
      return;
    }

    if (data[0] != static_cast<uint8_t>(foxglove::BinaryOpcode::SERVICE_CALL_RESPONSE)) {
      fmt::print("Received message with opcode {} is not a service call response.\n", data[0]);
      return;
    }

    printBinary(data, length);

    foxglove::ServiceResponse response;
    try {
      {
        // TODO(mfehr): This random 1 byte offset at the start does not make an aweful lot of sense and likely
        // is a bug in the ws-protocol library.
        const uint8_t* payload = data;
        const size_t payload_size = length;
        fmt::print("Payload size: {}\n", payload_size);
        size_t offset = 1;
        response.serviceId = foxglove::ReadUint32LE(payload + offset);
        offset += 4;
        response.callId = foxglove::ReadUint32LE(payload + offset);
        offset += 4;
        const size_t encoding_length = static_cast<size_t>(foxglove::ReadUint32LE(payload + offset));
        offset += 4;
        fmt::print("Offset: {}, encoding_length: {}\n", offset, encoding_length);
        response.encoding = std::string(reinterpret_cast<const char*>(payload + offset), encoding_length);
        offset += encoding_length;
        fmt::print("Offset: {}, Payload size: {}\n", offset, payload_size);
        const auto data_size = payload_size - offset;
        if (data_size > 0) {
          fmt::print("Data size: {}\n", data_size);
          response.data.resize(data_size);
          std::memcpy(response.data.data(), payload + offset, data_size);
        } else {
          fmt::print("No data to copy.\n");
        }
      }
      // response.read(data, length);
    } catch (const std::exception& e) {
      fmt::print("Failed to deserialize service response: {}\n", e.what());
      return;
    }

    try {
      responses[response.callId] = response;
    } catch (const std::exception& e) {
      fmt::print("Failed to store service response: {}\n", e.what());
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
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - it->second).count();
      fmt::print("Service call {} took {} ms\n", response.callId, duration);
      call_id_to_start_time.erase(it);
    } else {
      fmt::print("Start time for call ID {} not found.\n", response.callId);
    }
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

  // Pick the first advertised service as target, unless its name starts with "topic_info"
  auto foxglove_service_pair = services.begin();
  while (foxglove_service_pair != services.end() &&
         foxglove_service_pair->second.name.starts_with("topic_info")) {
    ++foxglove_service_pair;
  }

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

  fmt::print("\nTargeting Service '{}' for stress testing\n", foxglove_service.name);

  for (int i = 0; i < 100 && !g_abort; ++i) {
    foxglove::ServiceRequest request;
    request.callId = static_cast<uint32_t>(i) + 1;  // Increment or generate unique call ID as needed
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
    call_id_to_start_time[request.callId] = std::chrono::steady_clock::now();
    client.sendServiceRequest(request);
    fmt::print("Service request {} sent.\n", request.callId);
  }

  fmt::print("Waiting for responses...\n");
  while (responses.size() < 100 && !g_abort) {
    std::this_thread::sleep_for(1s);
    printOverviewTable(responses, 1, 100);
  }

  fmt::print("Closing client...\n");
  client.close();
  fmt::print("Done.\n");
  return 0;
}
