#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/base.h>
#include <foxglove/websocket/serialization.hpp>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/utils/stack_trace.h>
#include <hephaestus/utils/ws_client.h>
#include <hephaestus/utils/ws_protocol.h>
#include <nlohmann/json_fwd.hpp>

using heph::ws::WsClientNoTls;
using heph::ws::WsServerAdvertisements;
using heph::ws::WsServerChannelId;
using heph::ws::WsServerClientBinaryOpCode;
using heph::ws::WsServerClientChannelAd;
using heph::ws::WsServerClientChannelId;
using heph::ws::WsServerSubscriptionId;

namespace {
std::atomic<bool> g_abort{ false };  // NOLINT

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
    printAdvertisedTopics(ws_server_ads);
    return;
  }

  fmt::println("Received unhandled JSON message: \n'''\n{}\n'''", json_msg);
}

void handleBinaryMessage(
    const uint8_t* data, size_t length, WsClientNoTls& client,
    std::map<WsServerChannelId, WsServerChannelId>& sub_to_pub_channel_map,
    std::map<WsServerSubscriptionId, WsServerChannelId>& subscription_id_to_channel_id_map) {
  if (data == nullptr || length < (1 + 4 + 8)) {  // NOLINT
    fmt::print("Received invalid message.\n");
    g_abort = true;
    return;
  }

  const uint8_t opcode = data[0];                                 // NOLINT
  const auto subscription_id = foxglove::ReadUint32LE(data + 1);  // NOLINT

  if (opcode != static_cast<uint8_t>(WsServerClientBinaryOpCode::MESSAGE_DATA)) {
    fmt::print("Received unhandled binary message with op code {}\n", std::to_string(opcode));
    g_abort = true;
    return;
  }

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

  // 1 byte for the opcode, 4 bytes for the subscription ID, 8 bytes for the timestamp
  const uint8_t* message_data_start = data + 1 + 4 + 8;  // NOLINT
  const size_t message_data_size = length - 1 - 4 - 8;

  client.publish(client_channel_id, message_data_start, message_data_size);
}

}  // namespace

auto main(int argc, char** argv) -> int try {
  const heph::utils::StackTrace stack_trace;
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  if (argc < 2) {
    fmt::println("Usage: {} <url> (e.g. ws://localhost:8765)", argv[0]);  // NOLINT
    return 1;
  }

  const std::string url = argv[1];  // NOLINT
  WsClientNoTls client;

  WsServerAdvertisements ws_server_ads;

  std::map<WsServerChannelId, WsServerChannelId> sub_to_pub_channel_map;
  std::map<WsServerSubscriptionId, WsServerChannelId> subscription_id_to_channel_id_map;

  // NOLINTNEXTLINE
  const auto on_open_handler = [&](websocketpp::connection_hdl) { fmt::println("Connected to {}", url); };

  // NOLINTNEXTLINE
  const auto on_close_handler = [&](websocketpp::connection_hdl) {
    fmt::println("Connection closed");
    g_abort = true;
  };
  const auto json_message_handler = [&](const std::string& json_msg) {
    handleJsonMessage(json_msg, ws_server_ads, sub_to_pub_channel_map);
  };

  const auto binary_message_handler = [&](const uint8_t* data, size_t length) {
    handleBinaryMessage(data, length, client, sub_to_pub_channel_map, subscription_id_to_channel_id_map);
  };

  if (std::signal(SIGINT, sigintHandler) == SIG_ERR) {
    fmt::print("Error setting up signal handler.\n");
    return 1;
  }

  client.setTextMessageHandler(json_message_handler);
  client.setBinaryMessageHandler(binary_message_handler);

  fmt::println("Connecting to {}...", url);
  client.connect(url, on_open_handler, on_close_handler);

  fmt::println("Waiting for topics to be advertised...");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  while (ws_server_ads.channels.empty() && !g_abort) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  fmt::println("Advertise a client-side topic for each server-side topic, just with the prefix: 'mirror/'");
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(1, 100);  // NOLINT
    const auto mirror_channel_offset = static_cast<uint32_t>(dis(gen) * 100);

    std::vector<WsServerClientChannelAd> client_ads;
    for (const auto& [channel_id, channel] : ws_server_ads.channels) {
      client_ads.emplace_back(WsServerClientChannelAd{
          .channelId = channel_id + mirror_channel_offset,
          .topic = "mirror/" + channel.topic,
          .encoding = channel.encoding,
          .schemaName = channel.schemaName,
          .schema = channel.schema,
          .schemaEncoding = channel.schemaEncoding,
      });
      sub_to_pub_channel_map[channel_id] = channel_id + mirror_channel_offset;
    }

    heph::ws::printClientChannelAds(client_ads);

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
  constexpr auto SLEEP_DURATION_MS = std::chrono::milliseconds(10);
  while (!g_abort) {
    std::this_thread::sleep_for(SLEEP_DURATION_MS);
  }

  fmt::println("Unsubscribing from all topics...");
  {
    std::vector<WsServerSubscriptionId> subscription_ids;
    subscription_ids.reserve(subscription_id_to_channel_id_map.size());
    for (const auto& [subscription_id, channel_id] : subscription_id_to_channel_id_map) {
      subscription_ids.push_back(subscription_id);
    }
    client.unsubscribe(subscription_ids);
  }

  fmt::println("Unadvertising all client-side topics...");
  {
    std::vector<WsServerClientChannelId> channel_ids;
    channel_ids.reserve(sub_to_pub_channel_map.size());
    for (const auto& [_, pub_channel_id] : sub_to_pub_channel_map) {
      channel_ids.push_back(pub_channel_id);
    }
    client.unadvertise(channel_ids);
  }

  fmt::println("Closing client...");
  client.close();
  fmt::println("Done.");
  return 0;
} catch (const std::exception& e) {
  fmt::print("Exception: {}\n", e.what());
  return 1;
}