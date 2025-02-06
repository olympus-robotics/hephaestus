//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ws_bridge/ws_bridge.h"

namespace heph::ws_bridge {

Bridge::Bridge(std::shared_ptr<ipc::zenoh::Session> session, const BridgeConfig& config)
  : config_(config), ws_server_(nullptr), ipc_graph_(nullptr) {
  // Initialize IPC Graph
  {
    ipc_graph_ = std::make_unique<IpcGraph>(IpcGraphConfig{
        .session = session,
        .topic_discovery_cb =
            [this](const std::string& topic, const heph::serdes::TypeInfo& type_info) {
              this->CallbackIpcGraphTopicFound(topic, type_info);
            },
        .topic_removal_cb = [this](const std::string& topic) { this->CallbackIpcGraphTopicDropped(topic); },
        .graph_update_cb = [this](IpcGraphState state) { this->CallbackIpcGraphUpdated(state); },
    });
  }

  // Initialize IPC Interface
  {
    // TODO(mfehr): Add implementation for the IPC interface
  }

  // Initialize WS Server
  {
    // Log handler
    const auto ws_server_log_handler = [this](WsServerLogLevel level, char const* msg) {
      this->CallbackWsServerLogHandler(level, msg);
    };

    // Prepare server options
    auto ws_server_options = GetWsServerOptions(config);

    // Create server
    ws_server_ = foxglove::ServerFactory::createServer<websocketpp::connection_hdl>(
        "WS Server", ws_server_log_handler, ws_server_options);
    CHECK(ws_server_);

    // Prepare server callbacks
    foxglove::ServerHandlers<websocketpp::connection_hdl> ws_server_hdlrs;
    // Implements foxglove::CAPABILITY_PUBLISH (this capability does not exist in the foxglove library, but it
    // would represent the basic ability to advertise and publish topics from the server side )
    ws_server_hdlrs.subscribeHandler = [this](auto&&... args) {
      this->CallbackWsServerSubscribe(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.unsubscribeHandler = [this](auto&&... args) {
      this->CallbackWsServerUnsubscribe(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_CONNECTION_GRAPH
    ws_server_hdlrs.subscribeConnectionGraphHandler = [this](auto&&... args) {
      this->CallbackWsServerSubscribeConnectionGraph(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_CLIENT_PUBLISH
    ws_server_hdlrs.clientAdvertiseHandler = [this](auto&&... args) {
      this->CallbackWsServerClientAdvertise(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.clientUnadvertiseHandler = [this](auto&&... args) {
      this->CallbackWsServerClientUnadvertise(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.clientMessageHandler = [this](auto&&... args) {
      this->CallbackWsServerClientMessage(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_SERVICES
    ws_server_hdlrs.serviceRequestHandler = [this](auto&&... args) {
      this->CallbackWsServerServiceRequest(std::forward<decltype(args)>(args)...);
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

Bridge::~Bridge() {
  // Destructor implementation
}

////////////////////////////////////
// Bridge Life-cycle [THREADSAFE] //
////////////////////////////////////

auto Bridge::start() -> std::future<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    // Start the IPC Graph
    ipc_graph_->Start();

    // Start the WS Server
    StartWsServer(config_);

    std::promise<void> promise;
    promise.set_value();
    return promise.get_future();
}

auto Bridge::stop() -> std::future<void> {
    std::lock_guard<std::mutex> lock(mutex_);
    // Stop the IPC Graph
    ipc_graph_->Stop();

    // Stop the WS Server
    StopWsServer();

    std::promise<void> promise;
    promise.set_value();
    return promise.get_future();
}

void Bridge::wait() const {
    // This function can be used to wait for the server to stop if needed
    // Currently, it does nothing as the start and stop functions are synchronous
}

/////////////////////////////////////////////////
// Websocket Server Interface [NOT THREADSAFE] //
/////////////////////////////////////////////////

void Bridge::StartWsServer(const BridgeConfig& config) {
  heph::log(heph::INFO, "[WS Bridge] - Starting WS Server...");
  ws_server_->start(config.ws_server_address, config.ws_server_listening_port);
  heph::log(heph::INFO, "[WS Bridge] - WS Server ONLINE");

  const uint16_t ws_server_actual_listening_port = ws_server_->getPort();
  CHECK_EQ(ws_server_actual_listening_port, config.ws_server_listening_port);
}

void Bridge::StopWsServer() {
  heph::log(heph::INFO, "[WS Bridge] - Stopping WS Server...");
  ws_server_->stop();
  heph::log(heph::INFO, "[WS Bridge] - WS Server OFFLINE");
}

void Bridge::UpdateWsServerConnectionGraph(const TopicsToTypesMap& topics_w_type,
                                           const TopicsToTypesMap& services_to_nodes,
                                           const TopicToNodesMap& topic_to_subs,
                                           const TopicToNodesMap& topic_to_pubs) {
  foxglove::MapOfSets topic_to_pub_node_map;
  foxglove::MapOfSets topic_to_sub_node_map;
  for (const auto& [topic_name, topic_type] : topics_w_type) {
    (void)topic_type;

    auto subscriber_names = topic_to_subs.at(topic_name);
    auto publisher_names = topic_to_pubs.at(topic_name);

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
  for (const auto& [service_name, node_name] : services_to_nodes) {
    service_to_node_map[service_name].insert(node_name);
  }

  ws_server_->updateConnectionGraph(topic_to_pub_node_map, topic_to_sub_node_map, service_to_node_map);

  heph::log(heph::INFO, "[WS Bridge] - Updated IPC connection graph");
}

//////////////////////////////////////
// IPC Graph Callbacks [THREADSAFE] //
//////////////////////////////////////
void Bridge::CallbackIpcGraphTopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)topic;
  (void)type_info;
  // Handle the topic discovery logic here
  // For example, you might want to log the discovery or update some internal state
  // Example:
  // LOG(INFO) << "Topic found: " << topic << " with type: " << type_info.name;
}

void Bridge::CallbackIpcGraphTopicDropped(const std::string& topic) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)topic;
  // Handle the topic removal logic here
  // For example, you might want to log the removal or update some internal state
  // Example:
  // LOG(INFO) << "Topic dropped: " << topic;
}

void Bridge::CallbackIpcGraphUpdated(IpcGraphState state) {
  std::lock_guard<std::mutex> lock(mutex_);
  UpdateWsServerConnectionGraph(state.topics_to_types_map, state.services_to_nodes_map,
                                state.topic_to_subscribers_map, state.topic_to_publishers_map);
}

/////////////////////////////////////////////
// Websocket Server Callbacks [THREADSAFE] //
/////////////////////////////////////////////

void Bridge::CallbackWsServerLogHandler(WsServerLogLevel level, char const* msg) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)level;
  (void)msg;
  // Handle the log message
  // Example:
  // LOG(INFO) << "WS Server Log [" << static_cast<int>(level) << "]: " << msg;
}

void Bridge::CallbackWsServerSubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)channel_type;
  (void)client_handle;
  // Handle the subscription logic here
  // Example:
  // LOG(INFO) << "Client subscribed to channel: " << channel_type;
}

void Bridge::CallbackWsServerUnsubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)channel_type;
  (void)client_handle;
  // Handle the unsubscription logic here
  // Example:
  // LOG(INFO) << "Client unsubscribed from channel: " << channel_type;
}

void Bridge::CallbackWsServerClientAdvertise(const foxglove::ClientAdvertisement& advertisement,
                                             WsServerClientHandle client_handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)advertisement;
  (void)client_handle;
  // Handle the client advertisement logic here
  // Example:
  // LOG(INFO) << "Client advertised: " << advertisement.topic;
}

void Bridge::CallbackWsServerClientUnadvertise(WsServerChannelId channel_type,
                                               WsServerClientHandle client_handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)channel_type;
  (void)client_handle;
  // Handle the client unadvertisement logic here
  // Example:
  // LOG(INFO) << "Client unadvertised channel: " << channel_type;
}

void Bridge::CallbackWsServerClientMessage(const foxglove::ClientMessage& message,
                                           WsServerClientHandle client_handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)message;
  (void)client_handle;
  // Handle the client message logic here
  // Example:
  // LOG(INFO) << "Client sent message on channel: " << message.channel_id;
}

void Bridge::CallbackWsServerServiceRequest(const foxglove::ServiceRequest& request,
                                            WsServerClientHandle client_handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)request;
  (void)client_handle;
  // Handle the service request logic here
  // Example:
  // LOG(INFO) << "Service request received: " << request.service_id;
}

void Bridge::CallbackWsServerSubscribeConnectionGraph(bool subscribe) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)subscribe;
  ws_server_subscribed_to_connection_graph_ = subscribe;

  if (subscribe) {
    heph::log(heph::INFO, "A client subscribed to the connection graph!");
  }
}

}  // namespace heph::ws_bridge
