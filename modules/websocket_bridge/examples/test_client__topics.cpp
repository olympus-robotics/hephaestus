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
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/utils/protobuf_serdes.h>
#include <hephaestus/utils/signal_handler.h>
#include <hephaestus/utils/stack_trace.h>
#include <hephaestus/utils/ws_client.h>
#include <hephaestus/utils/ws_protocol.h>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;
using namespace heph::ws;

static std::atomic<bool> g_abort{ false };

void sigintHandler(int signal) {
  fmt::println("Received signal: {}", signal);
  g_abort = true;
}

void handleJsonMessage(const std::string& json_msg, WsServerAdvertisements& ws_server_ads,
                       std::map<WsServerChannelId, WsServerChannelId>& sub_to_pub_channel_map) {
  (void)sub_to_pub_channel_map;

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
    fmt::print("Received WS server advertisements.\n");  // Add this line
    return;
  }

  fmt::println("Received unhandled JSON message: \n'''\n{}\n'''", json_msg);
}

void handleBinaryMessage(
    const uint8_t* data, size_t length, WsClient& client,
    std::map<WsServerChannelId, WsServerChannelId>& sub_to_pub_channel_map,
    std::map<WsServerSubscriptionId, WsServerChannelId>& subscription_id_to_channel_id_map) {
  if (data == nullptr || length == 0) {
    fmt::print("Received invalid message.\n");
    g_abort = true;
    return;
  }

  const uint8_t opcode = data[0];

  if (opcode != static_cast<uint8_t>(WsServerBinaryOpCode::MESSAGE_DATA)) {
    fmt::println("Received unhandled binary message with op code {}", opcode);
    g_abort = true;
    return;
  }

  // Skipping the first byte (opcode)
  const uint8_t* payload = data + 1;

  const auto subscription_id = foxglove::ReadUint32LE(payload);

  // Find the server-side channel ID for this subscription ID.
  auto sub_id_it = subscription_id_to_channel_id_map.find(subscription_id);
  if (sub_id_it == subscription_id_to_channel_id_map.end()) {
    fmt::print("No matching channel ID found for subscription ID {}\n", subscription_id);
    g_abort = true;
    return;
  }
  const auto server_channel_id = sub_id_it->second;

  // Get the advertised client-side channel ID for this server-side channel ID.
  auto channel_it = sub_to_pub_channel_map.find(server_channel_id);
  if (channel_it == sub_to_pub_channel_map.end()) {
    fmt::print("No matching client channel ID found for server channel ID {}\n", server_channel_id);
    g_abort = true;
    return;
  }
  const auto client_channel_id = channel_it->second;

  // 1 byte for the opcode, 4 bytes for the subscription ID
  const uint8_t* message_data_start = data + 1 + 5;
  const size_t message_data_size = length - 1 - 4;

  client.publish(client_channel_id, message_data_start, message_data_size);
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

  std::map<WsServerChannelId, WsServerChannelId> sub_to_pub_channel_map;
  std::map<WsServerSubscriptionId, WsServerChannelId> subscription_id_to_channel_id_map;

  const auto on_open_handler = [&](websocketpp::connection_hdl) { fmt::println("Connected to {}", url); };

  const auto on_close_handler = [&](websocketpp::connection_hdl) {
    fmt::println("Connection closed");
    g_abort = true;
  };
  const auto json_message_handler = [&](const std::string& jsonMsg) {
    handleJsonMessage(jsonMsg, ws_server_ads, sub_to_pub_channel_map);
  };

  const auto binary_message_handler = [&](const uint8_t* data, size_t length) {
    handleBinaryMessage(data, length, client, sub_to_pub_channel_map, subscription_id_to_channel_id_map);
  };

  std::signal(SIGINT, sigintHandler);

  client.setTextMessageHandler(json_message_handler);
  client.setBinaryMessageHandler(binary_message_handler);

  fmt::println("Connecting to {}...", url);
  client.connect(url, on_open_handler, on_close_handler);

  fmt::println("Waiting for topics to be advertised...");
  std::this_thread::sleep_for(1s);
  while (ws_server_ads.channels.empty() && !g_abort) {
    std::this_thread::sleep_for(1s);
  }

  printAdvertisedTopics(ws_server_ads);

  fmt::println("Advertise a client-side topic for each server-side topic, just with the prefix: 'mirror/'");
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(1, 100);
    const uint32_t MIRROR_CHANNEL_ID_OFFSET = static_cast<uint32_t>(dis(gen) * 100);

    std::vector<WsServerClientChannelAd> client_ads;
    for (const auto& [channel_id, channel] : ws_server_ads.channels) {
      client_ads.emplace_back(WsServerClientChannelAd{
          channel_id + MIRROR_CHANNEL_ID_OFFSET,
          "mirror/" + channel.topic,
          channel.encoding,
          channel.schemaName,
          channel.schema,
          channel.schemaEncoding,
      });
      sub_to_pub_channel_map[channel_id] = channel_id + MIRROR_CHANNEL_ID_OFFSET;
    }
    client.advertise(client_ads);
  }

  fmt::println("Subscribe to all advertised server-channels, so we can start mirroring...");
  {
    std::vector<std::pair<WsServerSubscriptionId, WsServerChannelId>> subscriptions;
    WsServerSubscriptionId subscriber_id = 1;
    for (const auto& [channel_id, channel] : ws_server_ads.channels) {
      subscriptions.emplace_back(subscriber_id, channel_id);
      subscription_id_to_channel_id_map[subscriber_id] = channel_id;
      ++subscriber_id;
    }
    client.subscribe(subscriptions);
  }
  fmt::println("Subscribed to {} server channels.", subscription_id_to_channel_id_map.size());

  // MAIN LOOP
  fmt::println("Mirroring topics until Ctrl-C...");
  while (!g_abort) {
    std::this_thread::sleep_for(10ms);
  }

  fmt::println("Unsubscribing from all topics...");
  {
    std::vector<WsServerSubscriptionId> subscription_ids;
    for (const auto& [subscription_id, channel_id] : subscription_id_to_channel_id_map) {
      subscription_ids.push_back(subscription_id);
    }
    client.unsubscribe(subscription_ids);
  }

  fmt::println("Unadvertising all client-side topics...");
  {
    std::vector<WsServerClientChannelId> channel_ids;
    for (const auto& [_, pub_channel_id] : sub_to_pub_channel_map) {
      channel_ids.push_back(pub_channel_id);
    }
    client.unadvertise(channel_ids);
  }

  fmt::println("Closing client...");
  client.close();
  fmt::println("Done.");
  return 0;
}