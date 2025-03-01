//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge.h"

#include <cstddef>
#include <iterator>

#include <hephaestus/utils/protobuf_serdes.h>

namespace heph::ws {

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

        .service_discovery_cb =
            [this](const std::string& service_name, const serdes::ServiceTypeInfo& service_type_info) {
              this->callback__IpcGraph__ServiceFound(service_name, service_type_info);
            },
        .service_removal_cb =
            [this](const std::string& service) { this->callback__IpcGraph__ServiceDropped(service); },

        .graph_update_cb = [this](const ipc::zenoh::EndpointInfo& info,
                                  IpcGraphState state) { this->callback__IpcGraph__Updated(info, state); },
    });
  }

  // Initialize IPC Interface
  {
    ipc_interface_ = std::make_unique<IpcInterface>(session, config_.zenoh_config);
  }

  // Initialize WS Server
  {
    // Log handler
    const auto ws_server_log_handler = [this](WsServerLogLevel level, char const* msg) {
      this->callback__WsServer__Log(level, msg);
    };

    // Create server
    ws_server_ = WsServerFactory::createServer<WsServerClientHandle>("WS Server", ws_server_log_handler,
                                                                     config.ws_server_config);
    CHECK(ws_server_);

    // Prepare server callbacks
    WsServerHandlers ws_server_hdlrs;
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

/////////////////////////
// WsBridge Life-cycle //
/////////////////////////

void WsBridge::start() {
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  fmt::println("[WS Bridge] - Config:\n"
               "==========================================================\n"
               "{}\n"
               "==========================================================",
               convertBridgeConfigToString(config_));

  fmt::println("[WS Bridge] - Starting ...");

  {
    fmt::println("[WS Server] - Starting ...");
    ws_server_->start(config_.ws_server_address, config_.ws_server_port);

    // This is a sanity check I found in the ros-foxglove bridge, so I assume under certain conditions
    // (probably a port collision), this can actually happen.
    // TODO(mfehr): Test this and verify.
    const uint16_t ws_server_actual_listening_port = ws_server_->getPort();
    CHECK_EQ(ws_server_actual_listening_port, config_.ws_server_port);
    fmt::println("[WS Server] - ONLINE");
  }

  ipc_graph_->start();

  ipc_interface_->start();

  fmt::println("[WS Bridge] - ONLINE");
}

void WsBridge::stop() {
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  fmt::println("[WS Bridge] - Stopping ...");

  ipc_interface_->stop();

  ipc_graph_->stop();

  {
    fmt::println("[WS Server] - Stopping ...");
    ws_server_->stop();
    fmt::println("[WS Server] - OFFLINE ...");
  }

  fmt::println("[WS Bridge] - OFFLINE");
}

/////////////////////////
// IPC Graph Callbacks //
/////////////////////////

void WsBridge::callback__IpcGraph__TopicFound(const std::string& topic,
                                              const heph::serdes::TypeInfo& type_info) {
  CHECK(ipc_graph_);
  fmt::println("[WS Bridge] - New topic '{}' [{}] will be added  ...", topic, type_info.name);

  if (state_.hasIpcTopicMapping(topic)) {
    state_.printBridgeState();
    heph::log(heph::WARN,
              fmt::format("[WS Bridge] - Topic is already advertized! There are likely multiple publishers!",
                          "topic", topic));
    return;
  }

  WsServerChannelInfo new_ws_server_channel = convertIpcTypeInfoToWsChannelInfo(topic, type_info);

  std::vector<WsServerChannelInfo> new_ws_server_channels{ new_ws_server_channel };
  auto new_channel_ids = ws_server_->addChannels(new_ws_server_channels);

  CHECK_EQ(new_channel_ids.size(), 1);
  const auto new_channel_id = new_channel_ids.front();

  state_.addWsChannelToIpcTopicMapping(new_channel_id, topic);
  fmt::println("[WS Bridge] - New topic '{}' [{}] added successfully.", topic, new_channel_id);
}

void WsBridge::callback__IpcGraph__TopicDropped(const std::string& topic) {
  fmt::println("[WS Bridge] - Topic '{}' will be dropped  ...", topic);
  if (!state_.hasIpcTopicMapping(topic)) {
    state_.printBridgeState();
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
  fmt::println("[WS Bridge] - Topic '{}' dropped successfully.", topic);
}

void WsBridge::callback__IpcGraph__ServiceFound(const std::string& service_name,
                                                const heph::serdes::ServiceTypeInfo& type_info) {
  fmt::println("[WS Bridge] - Service '{}' [{}/{}] will be added  ...", service_name, type_info.request.name,
               type_info.reply.name);

  WsServerServiceInfo new_ws_server_service = {
    .name = service_name,
    // This interface was built with the ROS2 convention in mind, that the request and reply types are two
    // pieces of a common type, hence the type name is the same with a _Request or _Reply suffix. This is not
    // the case for hephaestus. So we just chose the request type name.
    .type = type_info.request.name,

    .request = std::make_optional<WsServerServiceRequestDefinition>(
        convertSerializationTypeToString(type_info.request.serialization), type_info.request.name,
        convertSerializationTypeToString(type_info.request.serialization),
        convertProtoBytesToFoxgloveBase64String(type_info.request.schema)),

    .response = std::make_optional<WsServerServiceResponseDefinition>(
        convertSerializationTypeToString(type_info.reply.serialization), type_info.reply.name,
        convertSerializationTypeToString(type_info.reply.serialization),
        convertProtoBytesToFoxgloveBase64String(type_info.reply.schema)),

    // NOTE: These seem to be legacy fields that are not used (anymore) in the foxglove library.
    .requestSchema = std::nullopt,
    .responseSchema = std::nullopt,
  };

  std::vector<WsServerServiceInfo> new_ws_server_services{ new_ws_server_service };
  auto new_service_ids = ws_server_->addServices(new_ws_server_services);

  CHECK_EQ(new_service_ids.size(), 1);
  const auto new_service_id = new_service_ids.front();

  state_.addWsServiceToIpcServiceMapping(new_service_id, service_name);

  fmt::println("[WS Bridge] - Service '{}' [{}] was added successfully.", service_name, new_service_id);
}

void WsBridge::callback__IpcGraph__ServiceDropped(const std::string& service_name) {
  fmt::println("[WS Bridge] - Service '{}' will be dropped  ...", service_name);

  if (!state_.hasIpcServiceMapping(service_name)) {
    state_.printBridgeState();
    heph::log(heph::WARN,
              fmt::format(
                  "[WS Bridge] - Service is already unadvertized! There are likely multiple service servers!",
                  "service_name", service_name));
    return;
  }

  const auto service_id = state_.getWsServiceForIpcService(service_name);
  state_.removeWsServiceToIpcServiceMapping(service_id, service_name);

  ws_server_->removeServices({ service_id });

  fmt::println("[WS Bridge] - Service '{}' dropped successfully.", service_name);
}

void WsBridge::callback__IpcGraph__Updated(const ipc::zenoh::EndpointInfo& info,
                                           IpcGraphState ipc_graph_state) {
  // TODO(mfehr): Not pretty but it works... Consider refactoring.
  std::string update_origin = "refresh";
  if (!info.topic.empty()) {
    const std::string endpoint_type = std::string(magic_enum::enum_name(info.type));
    update_origin = info.type == ipc::zenoh::EndpointType::PUBLISHER ||
                            info.type == ipc::zenoh::EndpointType::SUBSCRIBER ?
                        "topic '" + info.topic + "' [" + endpoint_type + "]" :
                        "service '" + info.topic + "' [" + endpoint_type + "]";
  }

  fmt::println("[WS Bridge] - Updating connection graph due to {}...", update_origin);

  ipc_graph_state.printIpcGraphState();
  CHECK(ipc_graph_state.checkConsistency());

  state_.printBridgeState();
  CHECK(state_.checkConsistency());

  foxglove::MapOfSets topic_to_pub_node_map;
  foxglove::MapOfSets topic_to_sub_node_map;
  foxglove::MapOfSets service_to_node_map;

  // Fill in topics to publishers and subscribers
  for (const auto& [topic_name, topic_type] : ipc_graph_state.topics_to_types_map) {
    (void)topic_type;

    auto publisher_names_it = ipc_graph_state.topic_to_publishers_map.find(topic_name);
    if (publisher_names_it != ipc_graph_state.topic_to_publishers_map.end()) {
      std::unordered_set<std::string> ws_server_pub_node_names;
      for (const auto& publisher_name : publisher_names_it->second) {
        ws_server_pub_node_names.insert(publisher_name);
      }
      topic_to_pub_node_map.emplace(topic_name, ws_server_pub_node_names);
    }

    auto subscriber_names_it = ipc_graph_state.topic_to_subscribers_map.find(topic_name);
    if (subscriber_names_it != ipc_graph_state.topic_to_subscribers_map.end()) {
      std::unordered_set<std::string> ws_server_sub_node_names;
      for (const auto& subscriber_name : subscriber_names_it->second) {
        ws_server_sub_node_names.insert(subscriber_name);
      }
      topic_to_sub_node_map.emplace(topic_name, ws_server_sub_node_names);
    }
  }

  // Fill in services to nodes
  for (const auto& [service_name, service_types] : ipc_graph_state.services_to_types_map) {
    (void)service_types;

    auto server_names_it = ipc_graph_state.services_to_server_map.find(service_name);
    if (server_names_it != ipc_graph_state.services_to_server_map.end()) {
      std::unordered_set<std::string> ws_server_node_names;
      for (const auto& server_name : server_names_it->second) {
        ws_server_node_names.insert(server_name);
      }
      service_to_node_map.emplace(service_name, ws_server_node_names);
    }
  }

  ws_server_->updateConnectionGraph(topic_to_pub_node_map, topic_to_sub_node_map, service_to_node_map);

  fmt::println("[WS Bridge] - Connection graph updated successfully.");
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

void WsBridge::callback__Ipc__ServiceResponsesReceived(
    WsServerServiceId service_id, WsServerServiceCallId call_id, const RawServiceResponses& responses,
    std::optional<ClientHandleWithName> client_handle_w_name_opt) {
  CHECK(ws_server_);

  if (!state_.hasWsServiceMapping(service_id)) {
    heph::log(heph::ERROR, "[WS Bridge] - Received service response for unknown or dropped service id!",
              "call_id", call_id, "service_id", service_id);
    return;
  }

  const std::string service_name = state_.getIpcServiceForWsService(service_id);

  bool sync_service_call = false;

  ClientHandleWithName client_handle_w_name;

  if (client_handle_w_name_opt.has_value()) {
    client_handle_w_name = client_handle_w_name_opt.value();
    sync_service_call = true;
  } else {
    // Look up client handle by call id
    const auto client_handle_w_name_lookup = state_.getClientForCallId(call_id);
    if (!client_handle_w_name_lookup.has_value()) {
      heph::log(heph::ERROR,
                "[WS Bridge] - No client handle found for service call id. Dropping service respose.",
                "call_id", call_id, "service_id", service_id);
      return;
    }
    client_handle_w_name = client_handle_w_name_lookup.value();
    state_.removeCallIdToClientMapping(call_id);
  }

  const std::string client_name = client_handle_w_name.second;
  const auto client_handle = client_handle_w_name.first;

  if (responses.empty()) {
    auto msg = fmt::format("[WS Bridge] - Timeout - no service responses received '{}' [{}]", service_name,
                           service_id);
    heph::log(heph::ERROR, msg, "service_id", service_id, "call_id", call_id);
    ws_server_->sendServiceFailure(client_handle, service_id, call_id, msg);
    return;
  }

  if (responses.size() > 1) {
    heph::log(heph::WARN, "[WS Bridge] - Multiple service responses received. Forwarding first only.",
              "response_count", responses.size(), "service_name", service_name, "service_id", service_id);
  }

  const auto& response = responses.front();
  if (response.topic != service_name) {
    auto msg = fmt::format("[WS Bridge] - Response and request names do not "
                           "match! '{}' vs '{}'",
                           response.topic, service_name);

    heph::log(heph::ERROR, msg, "service_id", service_id, "call_id", call_id, "response_topic",
              response.topic, "service_name", service_name);
    ws_server_->sendServiceFailure(client_handle, service_id, call_id, msg);
    return;
  }

  WsServerServiceResponse ws_server_response;
  if (!convertIpcRawServiceResponseToWsServiceResponse(service_id, call_id, response, ws_server_response)) {
    auto msg = fmt::format("[WS Bridge] - Failed to convert IPC service response "
                           "to WS service response for service '{}' [{}]",
                           service_name, service_id);
    heph::log(heph::ERROR, msg, "service_id", service_id, "call_id", call_id, "service_name", service_name);
    ws_server_->sendServiceFailure(client_handle, service_id, call_id, msg);
    return;
  }

  ws_server_->sendServiceResponse(client_handle, ws_server_response);

  fmt::println("[WS Bridge] - Client '{}' received service response '{}' [{}/{}] successfully. {}",
               client_name, service_name, service_id, call_id, sync_service_call ? "[SYNC]" : "[ASYNC]");
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
      heph::log(heph::FATAL, fmt::format("[WS Server] - {}", msg));
      break;
  }
}

void WsBridge::callback__WsServer__Subscribe(WsServerChannelId channel_id,
                                             WsServerClientHandle client_handle) {
  CHECK(ipc_graph_);
  CHECK(ipc_interface_);
  CHECK(ws_server_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const std::string topic = state_.getIpcTopicForWsChannel(channel_id);

  fmt::println("[WS Bridge] - Client '{}' subscribes to topic '{}' [{}] ...", client_name, topic, channel_id);

  state_.addWsChannelToClientMapping(channel_id, client_handle, client_name);

  if (ipc_interface_->hasSubscriber(topic)) {
    fmt::println("[WS Bridge] - Client '{}' subcribed to topic '{}' [{}] successfully [IPC SUB EXISTS].",
                 client_name, topic, channel_id);
    return;
  }

  std::optional<heph::serdes::TypeInfo> topic_type_info = ipc_graph_->getTopicTypeInfo(topic);

  if (!topic_type_info.has_value()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not subscribe because failed to retrieve type", "topic",
              topic, "channel_id", channel_id);
    return;
  }

  ipc_interface_->addSubscriber(topic, topic_type_info.value(),
                                [this](const heph::ipc::zenoh::MessageMetadata& metadata,
                                       std::span<const std::byte> data,
                                       const heph::serdes::TypeInfo& type_info) {
                                  this->callback__Ipc__MessageReceived(metadata, data, type_info);
                                });

  fmt::println("[WS Bridge] - Client '{}' subcribed to topic '{}' [{}] successfully. [IPC SUB ADDED]",
               client_name, topic, channel_id);

  state_.printBridgeState();
}

void WsBridge::callback__WsServer__Unsubscribe(WsServerChannelId channel_id,
                                               WsServerClientHandle client_handle) {
  CHECK(ipc_interface_);
  CHECK(ws_server_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const std::string topic = state_.getIpcTopicForWsChannel(channel_id);

  fmt::println("[WS Bridge] - Client '{}' unsubscribes from topic '{}' [{}] ...", client_name, topic,
               channel_id);

  state_.removeWsChannelToClientMapping(channel_id, client_handle);

  if (!state_.hasWsChannelWithClients(channel_id)) {
    if (ipc_interface_->hasSubscriber(topic)) {
      ipc_interface_->removeSubscriber(topic);
      fmt::println(
          "[WS Bridge] - Client '{}' unsubscribed from topic '{}' [{}] successfully. [IPC SUB REMOVED]",
          client_name, topic, channel_id);
    } else {
      fmt::println(
          "[WS Bridge] - Client '{}' unsubscribed from topic '{}' [{}] successfully. [IPC SUB NOT FOUND]",
          client_name, topic, channel_id);
    }
  } else {
    fmt::println(
        "[WS Bridge] - Client '{}' unsubscribed from topic '{}' [{}] successfully. [IPC SUB STILL NEEDED]",
        client_name, topic, channel_id);
  }

  state_.printBridgeState();
}

void WsBridge::callback__WsServer__ClientAdvertise(const WsServerClientChannelAd& advertisement,
                                                   WsServerClientHandle client_handle) {
  CHECK(ipc_graph_);
  CHECK(ipc_interface_);
  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const auto& topic = advertisement.topic;
  const auto& channel_id = advertisement.channelId;

  fmt::println("[WS Bridge] - Client '{}' advertises topic  '{}' [{}] ...", client_name, topic, channel_id);

  if (state_.hasClientChannelMapping(channel_id)) {
    heph::log(heph::ERROR, "[WS Bridge] - Client tried to advertise topic but the channel already exists!",
              "client_name", client_name, "channel_id", channel_id, "topic", topic);
    return;
  }

  if (state_.hasTopicToClientChannelMapping(topic)) {
    auto other_channels = state_.getClientChannelsForTopic(topic);
    const std::string other_channels_str = std::accumulate(
        std::next(other_channels.begin()), other_channels.end(), std::to_string(*other_channels.begin()),
        [](std::string a, int b) { return std::move(a) + ", " + std::to_string(b); });
    heph::log(heph::WARN, "[WS Bridge] - Multiple clients advertise the same topic!", "client_name",
              client_name, "channel_id", channel_id, "topic", topic, "num_clients",
              (other_channels.size() + 1), "other_channel_ids", other_channels_str);
  }

  state_.addClientChannelToTopicMapping(channel_id, topic);
  state_.addClientChannelToClientMapping(channel_id, client_handle, client_name);

  auto type_info = convertWsChannelInfoToIpcTypeInfo(advertisement);

  if (!type_info.has_value()) {
    heph::log(heph::ERROR, "[WS Bridge] - Failed to convert client advertisement to valid IPC type info!",
              "topic", topic, "channel_id", channel_id);
    return;
  }

  if (!ipc_interface_->hasPublisher(topic)) {
    ipc_interface_->addPublisher(topic, type_info.value());
    fmt::println("[WS Bridge] - Client '{}' advertised topic '{}' [{}] successfully [IPC PUB ADDED].",
                 client_name, topic, channel_id);
  } else {
    fmt::println("[WS Bridge] - Client '{}' advertised topic '{}' [{}] successfully [IPC PUB EXISTS].",
                 client_name, topic, channel_id);
  }

  state_.printBridgeState();
}

void WsBridge::callback__WsServer__ClientUnadvertise(WsServerClientChannelId client_channel_id,
                                                     WsServerClientHandle client_handle) {
  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  auto topic = state_.getTopicForClientChannel(client_channel_id);

  fmt::println("[WS Bridge] - Client '{}' unadvertises topic '{}' [{}] ...", client_name, topic,
               client_channel_id);

  if (topic.empty()) {
    heph::log(heph::ERROR, "[WS Bridge] - Client tried to unadvertise channel but the channel is unknown!",
              "client_name", client_name, "channel_id", client_channel_id);
    return;
  }

  auto client_handle_with_name = state_.getClientForClientChannel(client_channel_id);

  if (!client_handle_with_name.has_value()) {
    heph::log(heph::ERROR,
              "[WS Bridge] - Client tried to unadvertise topic but the channel is not owned by this client!",
              "client_name", client_name, "channel_id", client_channel_id, "topic", topic,
              "owner_client_name", client_handle_with_name.value().second);
    return;
  }

  state_.removeClientChannelToTopicMapping(client_channel_id);
  state_.removeClientChannelToClientMapping(client_channel_id);

  if (!state_.hasClientChannelsForTopic(topic)) {
    if (ipc_interface_->hasPublisher(topic)) {
      ipc_interface_->removePublisher(topic);
      fmt::println("[WS Bridge] - Client '{}' unadvertised topic '{}' [{}] successfully. [IPC PUB REMOVED]",
                   client_name, topic, client_channel_id);
    } else {
      fmt::println("[WS Bridge] - Client '{}' unadvertised topic '{}' [{}] successfully. [IPC PUB NOT FOUND]",
                   client_name, topic, client_channel_id);
    }
  } else {
    fmt::println(
        "[WS Bridge] - Client '{}' unadvertised topic '{}' [{}] successfully. [IPC PUB STILL NEEDED]",
        client_name, topic, client_channel_id);
  }

  state_.printBridgeState();
}

void WsBridge::callback__WsServer__ClientMessage(const WsServerClientMessage& message,
                                                 WsServerClientHandle client_handle) {
  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const auto& topic = message.advertisement.topic;
  const auto& channel_id = message.advertisement.channelId;

  if (!ipc_interface_->hasPublisher(topic)) {
    heph::log(heph::ERROR, "[WS Bridge] - Client sent message for unadvertised topic!", "client_name",
              client_name, "topic", topic);
    return;
  }

  if (message.data.empty()) {
    heph::log(heph::ERROR, "[WS Bridge] - Client sent empty message!", "client_name", client_name, "topic",
              topic, "channel_id", channel_id);
    return;
  }

  // Check if the message has enough data for opcode (1 byte) + channel ID (4 bytes)
  if (message.data.size() < 5) {
    heph::log(heph::ERROR, "[WS Bridge] - Client sent message with insufficient data!", "client_name",
              client_name, "topic", topic, "channel_id", channel_id, "message_size", message.data.size());
    return;
  }

  // 1 byte opcode + 4 bytes channel ID
  // The rest of the message is the payload
  const auto opcode = static_cast<uint8_t>(message.data[0]);
  const auto parsed_channel_id =
      static_cast<WsServerChannelId>(foxglove::ReadUint32LE(message.data.data() + 1));

  if (opcode != static_cast<uint8_t>(WsServerClientBinaryOpCode::MESSAGE_DATA)) {
    heph::log(heph::ERROR, "[WS Bridge] - Client sent message with unexpected opcode!", "client_name",
              client_name, "topic", topic, "channel_id", channel_id, "opcode", static_cast<int>(opcode));
    return;
  }
  if (parsed_channel_id != channel_id) {
    heph::log(heph::ERROR, "[WS Bridge] - Client sent message with unexpected channel id!", "client_name",
              client_name, "topic", topic, "channel_id", channel_id, "parsed_channel_id", parsed_channel_id);
    return;
  }

  // Forward only the payload (skip the bytes we extracted above)
  std::span<const std::byte> message_data = std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(message.data.data() + (1 + 4)), message.data.size() - (1 + 4));

  if (!ipc_interface_->publishMessage(topic, message_data)) {
    heph::log(heph::ERROR, "[WS Bridge] - Failed to publish client message!", "client_name", client_name,
              "topic", topic, "channel_id", channel_id);
  }
}

void WsBridge::callback__WsServer__ServiceRequest(const WsServerServiceRequest& request,
                                                  WsServerClientHandle client_handle) {
  CHECK(ipc_interface_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);

  if (!state_.hasWsServiceMapping(request.serviceId)) {
    auto msg = fmt::format("[WS Bridge] - Client '{}' sent service request for unadvertised service [{}/{}]!",
                           client_name, request.serviceId, request.callId);
    heph::log(heph::ERROR, msg, "service_id", request.serviceId, "call_id", request.callId, "client_name",
              client_name);
    ws_server_->sendServiceFailure(client_handle, request.serviceId, request.callId, msg);
    return;
  }

  if (request.encoding != "protobuf") {
    auto msg = fmt::format("[WS Bridge] - Client '{}' sent service request [{}/{}] with unsuported "
                           "encoding ({})!",
                           client_name, request.serviceId, request.callId, request.encoding);
    heph::log(heph::ERROR, msg, "service_id", request.serviceId, "call_id", request.callId, "client_name",
              client_name);
    ws_server_->sendServiceFailure(client_handle, request.serviceId, request.callId, msg);
    return;
  }

  auto service_name = state_.getIpcServiceForWsService(request.serviceId);
  const WsServerServiceId service_id = request.serviceId;
  const WsServerServiceCallId call_id = request.callId;

  fmt::println("[WS Bridge] - Client '{}' sent service request '{}' [{}/{}] ...", client_name, service_name,
               service_id, call_id);

  ipc::TopicConfig topic_config(service_name);

  std::span<const std::byte> buffer = std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(request.data.data()), request.data.size());

  const std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(config_.ipc_service_call_timeout_ms);

  if (config_.ipc_service_service_request_async) {
    ///////////
    // ASYNC //
    ///////////

    state_.addCallIdToClientMapping(call_id, client_handle, client_name);

    auto response_callback = [this, service_id, call_id](const RawServiceResponses& responses) -> void {
      fmt::println("[WS Bridge] - Service response (#{}) callback triggered for service [{}/{}] [ASYNC]",
                   responses.size(), service_id, call_id);

      callback__Ipc__ServiceResponsesReceived(service_id, call_id, responses);
    };

    auto future =
        ipc_interface_->callServiceAsync(call_id, topic_config, buffer, timeout_ms, response_callback);

    // NOTE: We do not wait for the future here, but we could, turning it into a synchronous call again.

    fmt::println("[WS Bridge] - Client '{}' service request for service '{}' [{}/{}] was dispached [ASYNC]",
                 client_name, service_name, service_id, call_id);
  } else {
    //////////
    // SYNC //
    //////////
    auto responses = ipc_interface_->callService(call_id, topic_config, buffer, timeout_ms);

    callback__Ipc__ServiceResponsesReceived(
        service_id, call_id, responses, std::make_optional<ClientHandleWithName>(client_handle, client_name));
  }
}

void WsBridge::callback__WsServer__SubscribeConnectionGraph(bool subscribe) {
  CHECK(ipc_graph_);

  if (subscribe) {
    fmt::println("[WS Bridge] - A client is subscribing to the connection graph ...");
    ipc_graph_->refreshConnectionGraph();
    fmt::println("[WS Bridge] - A client has subscribed to the connection graph successfully.");
  } else {
    fmt::println("[WS Bridge] - A client has unsubscribed from the connection graph.");
  }
}

}  // namespace heph::ws
