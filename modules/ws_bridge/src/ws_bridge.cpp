//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ws_bridge/ws_bridge.h"

namespace heph::ws_bridge {

WsBridge::WsBridge(std::shared_ptr<ipc::zenoh::Session> session, const WsBridgeConfig& config)
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

  {
    spinner_ =
        std::make_unique<concurrency::Spinner>(concurrency::Spinner::createNeverStoppingCallback(
                                                   [this] { heph::log(heph::INFO, state_.toString()); }),
                                               config_.ipc_spin_rate_hz);
  }
}

WsBridge::~WsBridge() {
  // Destructor implementation
}

//////////////////////////////////////
// WsBridge Life-cycle [THREADSAFE] //
//////////////////////////////////////

auto WsBridge::start() -> std::future<void> {
  absl::MutexLock lock(&mutex_);
  CHECK(spinner_);
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  ipc_graph_->Start();

  StartWsServer(config_);

  spinner_->start();

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

auto WsBridge::stop() -> std::future<void> {
  absl::MutexLock lock(&mutex_);
  CHECK(spinner_);
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  ipc_graph_->Stop();

  StopWsServer();

  return spinner_->stop();
}

void WsBridge::wait() const {
  absl::MutexLock lock(&mutex_);
  CHECK(spinner_);
  spinner_->wait();
}

/////////////////////////////////////////////////
// Websocket Server Interface [NOT THREADSAFE] //
/////////////////////////////////////////////////

void WsBridge::StartWsServer(const WsBridgeConfig& config) {
  heph::log(heph::INFO, "[WS WsBridge] - Starting WS Server...");
  ws_server_->start(config.ws_server_address, config.ws_server_listening_port);
  heph::log(heph::INFO, "[WS WsBridge] - WS Server ONLINE");

  const uint16_t ws_server_actual_listening_port = ws_server_->getPort();
  CHECK_EQ(ws_server_actual_listening_port, config.ws_server_listening_port);
}

void WsBridge::StopWsServer() {
  heph::log(heph::INFO, "[WS WsBridge] - Stopping WS Server...");
  ws_server_->stop();
  heph::log(heph::INFO, "[WS WsBridge] - WS Server OFFLINE");
}

void WsBridge::UpdateWsServerConnectionGraph(const TopicsToTypesMap& topics_w_type,
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

  heph::log(heph::INFO, "[WS WsBridge] - Updated IPC connection graph");
}

//////////////////////////////////////
// IPC Graph Callbacks [THREADSAFE] //
//////////////////////////////////////
void WsBridge::CallbackIpcGraphTopicFound(const std::string& topic, const heph::serdes::TypeInfo& type_info) {
  absl::MutexLock lock(&mutex_);
  (void)topic;
  (void)type_info;
  // Handle the topic discovery logic here
  // For example, you might want to log the discovery or update some internal state
  // Example:
  // LOG(INFO) << "Topic found: " << topic << " with type: " << type_info.name;
}

void WsBridge::CallbackIpcGraphTopicDropped(const std::string& topic) {
  absl::MutexLock lock(&mutex_);
  (void)topic;
  // Handle the topic removal logic here
  // For example, you might want to log the removal or update some internal state
  // Example:
  // LOG(INFO) << "Topic dropped: " << topic;
}

void WsBridge::CallbackIpcGraphUpdated(IpcGraphState state) {
  absl::MutexLock lock(&mutex_);
  UpdateWsServerConnectionGraph(state.topics_to_types_map, state.services_to_nodes_map,
                                state.topic_to_subscribers_map, state.topic_to_publishers_map);
}

/////////////////////////////////////////////
// Websocket Server Callbacks [THREADSAFE] //
/////////////////////////////////////////////

void WsBridge::CallbackWsServerLogHandler(WsServerLogLevel level, char const* msg) {
  switch (level) {
    case WsServerLogLevel::Debug:
      heph::log(heph::DEBUG, fmt::format("[WS Server] - {}", msg));
      break;
    case WsServerLogLevel::Info:
      heph::log(heph::INFO, fmt::format("[WS Server] - {}", msg));
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

void WsBridge::CallbackWsServerSubscribe(WsServerChannelId channel_type, WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_);
  (void)channel_type;
  (void)client_handle;
  // Handle the subscription logic here
  // Example:
  // LOG(INFO) << "Client subscribed to channel: " << channel_type;
}

void WsBridge::CallbackWsServerUnsubscribe(WsServerChannelId channel_type,
                                           WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_);
  (void)channel_type;
  (void)client_handle;
  // Handle the unsubscription logic here
  // Example:
  // LOG(INFO) << "Client unsubscribed from channel: " << channel_type;
}

void WsBridge::CallbackWsServerClientAdvertise(const foxglove::ClientAdvertisement& advertisement,
                                               WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_);
  (void)advertisement;
  (void)client_handle;
  // Handle the client advertisement logic here
  // Example:
  // LOG(INFO) << "Client advertised: " << advertisement.topic;
}

void WsBridge::CallbackWsServerClientUnadvertise(WsServerChannelId channel_type,
                                                 WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_);
  (void)channel_type;
  (void)client_handle;
  // Handle the client unadvertisement logic here
  // Example:
  // LOG(INFO) << "Client unadvertised channel: " << channel_type;
}

void WsBridge::CallbackWsServerClientMessage(const foxglove::ClientMessage& message,
                                             WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_);
  (void)message;
  (void)client_handle;
  // Handle the client message logic here
  // Example:
  // LOG(INFO) << "Client sent message on channel: " << message.channel_id;
}

void WsBridge::CallbackWsServerServiceRequest(const foxglove::ServiceRequest& request,
                                              WsServerClientHandle client_handle) {
  absl::MutexLock lock(&mutex_);
  (void)request;
  (void)client_handle;
  // Handle the service request logic here
  // Example:
  // LOG(INFO) << "Service request received: " << request.service_id;
}

void WsBridge::CallbackWsServerSubscribeConnectionGraph(bool subscribe) {
  absl::MutexLock lock(&mutex_);
  (void)subscribe;
  ws_server_subscribed_to_connection_graph_ = subscribe;

  if (subscribe) {
    heph::log(heph::INFO, "A client subscribed to the connection graph!");
  }
}

}  // namespace heph::ws_bridge
