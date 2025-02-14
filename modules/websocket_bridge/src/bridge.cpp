//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge.h"

namespace heph::ws_bridge {

WsBridge::WsBridge(std::shared_ptr<ipc::zenoh::Session> session, const WsBridgeConfig& config)
  : config_(config), ws_server_(nullptr), ipc_graph_(nullptr) {
  // Initialize IPC Graph
  {
    ipc_graph_ = std::make_unique<IpcGraph>(IpcGraphConfig{
        .session = session,
        .topic_discovery_cb =
            [this](const std::string& topic, const heph::serdes::TypeInfo& type_info) {
              this->callback__IpcGraph__TopicFound(topic, type_info);
            },
        .topic_removal_cb =
            [this](const std::string& topic) { this->callback__IpcGraph__TopicDropped(topic); },
        .graph_update_cb = [this](IpcGraphState state) { this->callback__IpcGraph__Updated(state); },
    });
  }

  // Initialize IPC Interface
  {
    ipc_interface_ = std::make_unique<IpcInterface>(session);
  }

  // Initialize WS Server
  {
    // Log handler
    const auto ws_server_log_handler = [this](WsServerLogLevel level, char const* msg) {
      this->callback__WsServer__Log(level, msg);
    };

    // Create server
    ws_server_ = foxglove::ServerFactory::createServer<websocketpp::connection_hdl>(
        "WS Server", ws_server_log_handler, config.ws_server_config);
    CHECK(ws_server_);

    // Prepare server callbacks
    foxglove::ServerHandlers<websocketpp::connection_hdl> ws_server_hdlrs;
    // Implements foxglove::CAPABILITY_PUBLISH (this capability does not exist in the foxglove library, but it
    // would represent the basic ability to advertise and publish topics from the server side )
    ws_server_hdlrs.subscribeHandler = [this](auto&&... args) {
      this->callback__WsServer__Subscribe(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.unsubscribeHandler = [this](auto&&... args) {
      this->callback__WsServer__Unsubscribe(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_CONNECTION_GRAPH
    ws_server_hdlrs.subscribeConnectionGraphHandler = [this](auto&&... args) {
      this->callback__WsServer__SubscribeConnectionGraph(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_CLIENT_PUBLISH
    ws_server_hdlrs.clientAdvertiseHandler = [this](auto&&... args) {
      this->callback__WsServer__ClientAdvertise(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.clientUnadvertiseHandler = [this](auto&&... args) {
      this->callback__WsServer__ClientUnadvertise(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.clientMessageHandler = [this](auto&&... args) {
      this->callback__WsServer__ClientMessage(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_SERVICES
    ws_server_hdlrs.serviceRequestHandler = [this](auto&&... args) {
      this->callback__WsServer__ServiceRequest(std::forward<decltype(args)>(args)...);
    };

    // TODO(mfehr): Add implementation for the following capabilities:
    // - foxglove::CAPABILITY_ASSETS
    // - foxglove::CAPABILITY_PARAMETERS
    // - foxglove::CAPABILITY_TIME
    // Reference implementation of both capabilities can be found in the ROS2
    // bridge (ros2_foxglove_bridge.cpp)

    ws_server_->setHandlers(std::move(ws_server_hdlrs));
  }
}

WsBridge::~WsBridge() {
  // Destructor implementation
}

void WsBridge::printBridgeState() const {
  fmt::print("{}\n", state_.toString());
}

/////////////////////////
// WsBridge Life-cycle //
/////////////////////////

void WsBridge::start() {
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  fmt::println("[WS Bridge] - Starting...");

  {
    fmt::println("[WS Server] - Starting...");
    ws_server_->start(config_.ws_server_address, config_.ws_server_listening_port);

    // This is a sanity check I found in the ros-foxglove bridge, so I assume under certain conditions
    // (probably a port collision), this can actually happen.
    // TODO(mfehr): Test this and verify.
    const uint16_t ws_server_actual_listening_port = ws_server_->getPort();
    CHECK_EQ(ws_server_actual_listening_port, config_.ws_server_listening_port);
    fmt::println("[WS Server] - ONLINE");
  }

  ipc_graph_->start();

  ipc_interface_->start();

  fmt::println("[WS Bridge] - ONLINE");
}

void WsBridge::stop() {
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  fmt::println("[WS Bridge] - Stopping...");

  ipc_interface_->stop();

  ipc_graph_->stop();

  {
    fmt::println("[WS Server] - Stopping...");
    ws_server_->stop();
    fmt::println("[WS Server] - OFFLINE...");
  }

  fmt::println("[WS Bridge] - OFFLINE");
}

/////////////////////////
// IPC Graph Callbacks //
/////////////////////////

void WsBridge::callback__IpcGraph__TopicFound(const std::string& topic,
                                              const heph::serdes::TypeInfo& type_info) {
  CHECK(ipc_graph_);

  if (state_.hasIpcTopicMapping(topic)) {
    printBridgeState();
    heph::log(heph::WARN,
              fmt::format("[WS Bridge] - Topic is already advertized! There are likely multiple publishers!",
                          "topic", topic));
    return;
  }

  const std::string schema_str = convertProtoBytesToFoxgloveBase64String(type_info.schema);

  const std::string schema_encoding_str = convertSerializationTypeToString(type_info.serialization);

  WsServerChannelInfo new_ws_server_channel{
    .topic = topic,
    .encoding = schema_encoding_str,
    .schemaName = type_info.name,
    .schema = schema_str,
    .schemaEncoding = schema_encoding_str,
  };

  // debugPrintSchema(type_info.schema);

  // heph::log(heph::INFO, "[WS Bridge] - New Channel", "topic", new_ws_server_channel.topic, "encoding",
  //           new_ws_server_channel.encoding, "schemaName", new_ws_server_channel.schemaName, "schema",
  //           new_ws_server_channel.schema, "schemaEncoding",
  //           new_ws_server_channel.schemaEncoding.value_or("N/A"));

  std::vector<WsServerChannelInfo> new_ws_server_channels{ new_ws_server_channel };
  auto new_channel_ids = ws_server_->addChannels(new_ws_server_channels);

  CHECK_EQ(new_channel_ids.size(), 1);
  const auto new_channel_id = new_channel_ids.front();

  state_.addWsChannelToIpcTopicMapping(new_channel_id, topic);
}

void WsBridge::callback__IpcGraph__TopicDropped(const std::string& topic) {
  (void)topic;
  if (!state_.hasIpcTopicMapping(topic)) {
    printBridgeState();
    heph::log(
        heph::WARN,
        fmt::format("[WS Bridge] - Topic is already unadvertized! There are likely multiple publishers!",
                    "topic", topic));
    return;
  }

  const auto channel_id = state_.getWsChannelForIpcTopic(topic);

  // Clean up IPC interface side
  {
    state_.removeWsChannelToIpcTopicMapping(channel_id, topic);

    if (ipc_interface_->hasSubscriber(topic)) {
      ipc_interface_->removeSubscriber(topic);
    }
  }

  // Clean up WS Server side
  {
    state_.removeWsChannelToClientMapping(channel_id);

    ws_server_->removeChannels({ channel_id });
  }
}

void WsBridge::callback__IpcGraph__Updated(IpcGraphState state) {
  printBridgeState();

  foxglove::MapOfSets topic_to_pub_node_map;
  foxglove::MapOfSets topic_to_sub_node_map;
  for (const auto& [topic_name, topic_type] : state.topics_to_types_map) {
    (void)topic_type;

    auto subscriber_names = state.topic_to_subscribers_map.at(topic_name);
    auto publisher_names = state.topic_to_publishers_map.at(topic_name);

    std::unordered_set<std::string> ws_server_pub_node_names;
    for (const auto& publisher_name : publisher_names) {
      ws_server_pub_node_names.insert(publisher_name);
    }
    topic_to_pub_node_map.emplace(topic_name, ws_server_pub_node_names);

    std::unordered_set<std::string> ws_server_sub_node_names;
    for (const auto& subscriber_name : subscriber_names) {
      ws_server_sub_node_names.insert(subscriber_name);
    }
    topic_to_sub_node_map.emplace(topic_name, ws_server_sub_node_names);
  }

  foxglove::MapOfSets service_to_node_map;
  for (const auto& [service_name, node_name] : state.services_to_nodes_map) {
    service_to_node_map[service_name].insert(node_name);
  }

  ws_server_->updateConnectionGraph(topic_to_pub_node_map, topic_to_sub_node_map, service_to_node_map);
}

/////////////////////////////
// IPC Interface Callbacks //
/////////////////////////////

void WsBridge::callback__Ipc__MessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
                                              std::span<const std::byte> message_data,
                                              const heph::serdes::TypeInfo& type_info) {
  (void)type_info;

  CHECK(ws_server_);

  const std::string topic = metadata.topic;
  const WsServerChannelId channel_id = state_.getWsChannelForIpcTopic(topic);

  auto clients = state_.getClientsForWsChannel(channel_id);
  if (!clients.has_value()) {
    return;
  }

  for (const auto& client_handle : clients.value()) {
    if (client_handle.first.expired()) {
      continue;
    }

    const uint64_t timestamp_now_ns =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count());
    ws_server_->sendMessage(client_handle.first, channel_id, timestamp_now_ns,
                            reinterpret_cast<const uint8_t*>(message_data.data()), message_data.size());
  }
}

////////////////////////////////
// Websocket Server Callbacks //
////////////////////////////////

void WsBridge::callback__WsServer__Log(WsServerLogLevel level, char const* msg) {
  switch (level) {
    case WsServerLogLevel::Debug:
      heph::log(heph::DEBUG, fmt::format("[WS Server] - {}", msg));
      break;
    case WsServerLogLevel::Info:
      heph::log(heph::DEBUG, fmt::format("[WS Server] - {}", msg));
      break;
    case WsServerLogLevel::Warn:
      heph::log(heph::WARN, fmt::format("[WS Server] - {}", msg));
      break;
    case WsServerLogLevel::Error:
      heph::log(heph::ERROR, fmt::format("[WS Server] - {}", msg));
      break;
    case WsServerLogLevel::Critical:
      heph::log(heph::ERROR, fmt::format("[WS Server] - CRITICAL - {}", msg));
      break;
  }
}

void WsBridge::callback__WsServer__Subscribe(WsServerChannelId channel_id,
                                             WsServerClientHandle client_handle) {
  CHECK(ipc_graph_);
  CHECK(ipc_interface_);
  CHECK(ws_server_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);

  state_.addWsChannelToClientMapping(channel_id, client_handle, client_name);

  const std::string topic = state_.getIpcTopicForWsChannel(channel_id);

  if (ipc_interface_->hasSubscriber(topic)) {
    return;
  }

  std::optional<heph::serdes::TypeInfo> topic_type_info = ipc_graph_->getTopicTypeInfo(topic);

  if (!topic_type_info.has_value()) {
    heph::log(
        heph::ERROR,
        fmt::format("[App Bridge] - '{}' ==> [{}] - Could not subscribe because failed to retrieve type!",
                    topic, channel_id));
    return;
  }

  ipc_interface_->addSubscriber(topic, topic_type_info.value(),
                                [this](const heph::ipc::zenoh::MessageMetadata& metadata,
                                       std::span<const std::byte> data,
                                       const heph::serdes::TypeInfo& type_info) {
                                  this->callback__Ipc__MessageReceived(metadata, data, type_info);
                                });

  printBridgeState();
}

void WsBridge::callback__WsServer__Unsubscribe(WsServerChannelId channel_id,
                                               WsServerClientHandle client_handle) {
  CHECK(ipc_interface_);
  CHECK(ws_server_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);

  state_.removeWsChannelToClientMapping(channel_id, client_handle);

  const std::string topic = state_.getIpcTopicForWsChannel(channel_id);
  if (!state_.hasWsChannelWithClients(channel_id)) {
    if (ipc_interface_->hasSubscriber(topic)) {
      ipc_interface_->removeSubscriber(topic);
    }
  }

  printBridgeState();
}

void WsBridge::callback__WsServer__ClientAdvertise(const foxglove::ClientAdvertisement& advertisement,
                                                   WsServerClientHandle client_handle) {
  (void)advertisement;
  (void)client_handle;
  // Handle the client advertisement logic here
  // Example:
  // LOG(INFO) << "Client advertised: " << advertisement.topic;

  printBridgeState();
}

void WsBridge::callback__WsServer__ClientUnadvertise(WsServerChannelId channel_id,
                                                     WsServerClientHandle client_handle) {
  (void)channel_id;
  (void)client_handle;
  // Handle the client unadvertisement logic here
  // Example:
  // LOG(INFO) << "Client unadvertised channel: " << channel_id;

  printBridgeState();
}

void WsBridge::callback__WsServer__ClientMessage(const foxglove::ClientMessage& message,
                                                 WsServerClientHandle client_handle) {
  (void)message;
  (void)client_handle;
  // Handle the client message logic here
  // Example:
  // LOG(INFO) << "Client sent message on channel: " << message.channel_id;
}

void WsBridge::callback__WsServer__ServiceRequest(const foxglove::ServiceRequest& request,
                                                  WsServerClientHandle client_handle) {
  (void)request;
  (void)client_handle;
  // Handle the service request logic here
  // Example:
  // LOG(INFO) << "Service request received: " << request.service_id;
}

void WsBridge::callback__WsServer__SubscribeConnectionGraph(bool subscribe) {
  CHECK(ipc_graph_);

  if (subscribe) {
    heph::log(heph::INFO, "A client subscribed to the connection graph!");
    ipc_graph_->refreshConnectionGraph();
  }
}

}  // namespace heph::ws_bridge
