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

static std::atomic<bool> g_abort{ false };

void sigintHandler(int signal) {
  fmt::println("Received signal: {}", signal);
  g_abort = true;
}

void handleJsonMessage(const std::string& json_msg, heph::ws_bridge::WsServerAdvertisements& ws_server_ads) {
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
    fmt::print("Received WS server advertisements.\n");  // Add this line
    return;
  }

  fmt::println("Received unhandled JSON message: \n'''\n{}\n'''", json_msg);
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

  const auto on_open_handler = [&](websocketpp::connection_hdl) { fmt::println("Connected to {}", url); };

  const auto on_close_handler = [&](websocketpp::connection_hdl) {
    fmt::println("Connection closed");
    g_abort = true;
  };
  const auto json_message_handler = [&](const std::string& jsonMsg) {
    handleJsonMessage(jsonMsg, ws_server_ads);
  };

  std::signal(SIGINT, sigintHandler);

  client.setTextMessageHandler(json_message_handler);

  fmt::println("Connecting to {}...", url);
  client.connect(url, on_open_handler, on_close_handler);

  std::this_thread::sleep_for(1s);

  fmt::println("Waiting for topics to be advertised...");
  while (ws_server_ads.channels.empty() && !g_abort) {
    std::this_thread::sleep_for(1s);
  }

  heph::ws_bridge::printAdvertisedTopics(ws_server_ads);

  fmt::println("Closing client...");
  client.close();
  fmt::println("Done.");
  return 0;
}