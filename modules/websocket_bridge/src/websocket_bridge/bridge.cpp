//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/bridge.h"

#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <absl/log/check.h>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <foxglove/websocket/serialization.hpp>
#include <foxglove/websocket/server_interface.hpp>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/ipc_graph.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/websocket_bridge/bridge_config.h"
#include "hephaestus/websocket_bridge/bridge_state.h"
#include "hephaestus/websocket_bridge/ipc/ipc_entity_manager.h"
#include "hephaestus/websocket_bridge/utils/protobuf_serdes.h"
#include "hephaestus/websocket_bridge/utils/ws_protocol.h"

namespace heph::ws {

WebsocketBridge::WebsocketBridge(const std::shared_ptr<ipc::zenoh::Session>& session,
                                 const WebsocketBridgeConfig& config)
  : config_(config), ws_server_(nullptr), ipc_graph_(nullptr) {
  // Initialize IPC Graph
  {
    ipc_graph_ = std::make_unique<IpcGraph>(
        IpcGraphConfig{
            .session = session,
            .track_topics_based_on_subscribers = config.ipc_advertise_topics_based_on_subscribers,
        },
        IpcGraphCallbacks{
            .topic_discovery_cb =
                [this](const std::string& topic, const heph::serdes::TypeInfo& type_info) {
                  if (shouldBridgeIpcTopic(topic, config_)) {
                    this->callback_IpcGraph_TopicFound(topic, type_info);
                  } else {
                    heph::log(
                        heph::INFO,
                        fmt::format("[WS Bridge] - Ignoring topic '{}' - not whitelisted, or blacklisted.",
                                    topic));
                  }
                },
            .topic_removal_cb =
                [this](const std::string& topic) {
                  if (shouldBridgeIpcTopic(topic, config_)) {
                    this->callback_IpcGraph_TopicDropped(topic);
                  }
                },
            .service_discovery_cb =
                [this](const std::string& service_name, const serdes::ServiceTypeInfo& service_type_info) {
                  if (shouldBridgeIpcService(service_name, config_)) {
                    this->callback_IpcGraph_ServiceFound(service_name, service_type_info);
                  } else {
                    heph::log(
                        heph::INFO,
                        fmt::format("[WS Bridge] - Ignoring service '{}' - not whitelisted, or blacklisted.",
                                    service_name));
                  }
                },
            .service_removal_cb =
                [this](const std::string& service) {
                  if (shouldBridgeIpcService(service, config_)) {
                    this->callback_IpcGraph_ServiceDropped(service);
                  }
                },
            .graph_update_cb =
                [this](const ipc::zenoh::EndpointInfo& info, const IpcGraphState& state) {
                  this->callback_IpcGraph_Updated(info, state);
                },
        });
  }

  // Initialize IPC Interface
  {
    ipc_entity_manager_ = std::make_unique<IpcEntityManager>(session, config_.zenoh_config);
  }

  // Initialize WS Server
  {
    // Log handler
    const auto ws_server_log_handler = [](WsLogLevel level, const char* msg) {
      WebsocketBridge::callback_Ws_Log(level, msg);
    };

    // Create server
    ws_server_ =
        WsFactory::createServer<WsClientHandle>("WS Server", ws_server_log_handler, config.ws_server_config);
    CHECK(ws_server_);

    // Prepare server callbacks
    WsHandlers ws_server_hdlrs;
    // Implements foxglove::CAPABILITY_PUBLISH (this capability does not exist in the foxglove library, but it
    // would represent the basic ability to advertise and publish topics from the server side )
    ws_server_hdlrs.subscribeHandler = [this](auto&&... args) {
      this->callback_Ws_Subscribe(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.unsubscribeHandler = [this](auto&&... args) {
      this->callback_Ws_Unsubscribe(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_CONNECTION_GRAPH
    ws_server_hdlrs.subscribeConnectionGraphHandler = [this](auto&&... args) {
      this->callback_Ws_SubscribeConnectionGraph(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_CLIENT_PUBLISH
    ws_server_hdlrs.clientAdvertiseHandler = [this](auto&&... args) {
      this->callback_Ws_ClientAdvertise(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.clientUnadvertiseHandler = [this](auto&&... args) {
      this->callback_Ws_ClientUnadvertise(std::forward<decltype(args)>(args)...);
    };
    ws_server_hdlrs.clientMessageHandler = [this](auto&&... args) {
      this->callback_Ws_ClientMessage(std::forward<decltype(args)>(args)...);
    };

    // Implements foxglove::CAPABILITY_SERVICES
    ws_server_hdlrs.serviceRequestHandler = [this](auto&&... args) {
      this->callback_Ws_ServiceRequest(std::forward<decltype(args)>(args)...);
    };

    // TODO: Add implementation for the following capabilities:
    // - foxglove::CAPABILITY_ASSETS
    // - foxglove::CAPABILITY_PARAMETERS
    // - foxglove::CAPABILITY_TIME
    // Reference implementation of those capabilities can be found in the ROS2
    // bridge (ros2_foxglove_bridge.cpp)

    ws_server_->setHandlers(std::move(ws_server_hdlrs));
  }
}

/////////////////////////
// WebsocketBridge Life-cycle //
/////////////////////////

void WebsocketBridge::start() {
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  fmt::println("\n[WS Bridge] - Config:\n"
               "==========================================================\n"
               "{}\n"
               "==========================================================",
               convertBridgeConfigToString(config_));

  heph::log(heph::INFO, "[WS Bridge] - Starting ...");

  {
    heph::log(heph::INFO, "[WS Server] - Starting ...");
    ws_server_->start(config_.ws_server_address, config_.ws_server_port);

    const uint16_t ws_server_actual_listening_port = ws_server_->getPort();
    CHECK_EQ(ws_server_actual_listening_port, config_.ws_server_port);
    heph::log(heph::INFO, "[WS Server] - ONLINE");
  }

  ipc_graph_->start();

  ipc_entity_manager_->start();

  heph::log(heph::INFO, "[WS Bridge] - ONLINE");
}

void WebsocketBridge::stop() {
  CHECK(ws_server_);
  CHECK(ipc_graph_);

  heph::log(heph::INFO, "[WS Bridge] - Stopping ...");

  ipc_entity_manager_->stop();

  ipc_graph_->stop();

  {
    heph::log(heph::INFO, "[WS Server] - Stopping ...");
    ws_server_->stop();
    heph::log(heph::INFO, "[WS Server] - OFFLINE ...");
  }

  heph::log(heph::INFO, "[WS Bridge] - OFFLINE");
}

/////////////////////////
// IPC Graph Callbacks //
/////////////////////////

void WebsocketBridge::callback_IpcGraph_TopicFound(const std::string& topic,
                                                   const heph::serdes::TypeInfo& type_info) {
  CHECK(ipc_graph_);
  heph::log(heph::INFO, "[WS Bridge] - New topic will be added  ...", "topic", topic, "type_name",
            type_info.name);

  if (state_.hasIpcTopicMapping(topic)) {
    if (config_.ws_server_verbose_bridge_state) {
      state_.printBridgeState();
    }
    heph::log(
        heph::WARN,
        fmt::format("\n[WS Bridge] - Topic is already advertized! There are likely multiple publishers!",
                    "topic", topic));
    return;
  }

  const WsChannelInfo new_ws_server_channel = convertIpcTypeInfoToWsChannelInfo(topic, type_info);

  const std::vector<WsChannelInfo> new_ws_server_channels{ new_ws_server_channel };
  auto new_channel_ids = ws_server_->addChannels(new_ws_server_channels);

  CHECK_EQ(new_channel_ids.size(), 1);
  const auto new_channel_id = new_channel_ids.front();

  state_.addWsChannelToIpcTopicMapping(new_channel_id, topic);
  heph::log(heph::INFO, "[WS Bridge] - New topic added successfully.", "topic", topic, "type_name",
            type_info.name, "channel_id", new_channel_id);
}

void WebsocketBridge::callback_IpcGraph_TopicDropped(const std::string& topic) {
  heph::log(heph::INFO, "[WS Bridge] - Topic will be dropped  ...", "topic", topic);
  if (!state_.hasIpcTopicMapping(topic)) {
    if (config_.ws_server_verbose_bridge_state) {
      state_.printBridgeState();
    }
    heph::log(
        heph::WARN,
        fmt::format("\n[WS Bridge] - Topic is already unadvertized! There are likely multiple publishers!",
                    "topic", topic));
    return;
  }

  const auto channel_id = state_.getWsChannelForIpcTopic(topic);

  // Clean up IPC interface side
  {
    state_.removeWsChannelToIpcTopicMapping(channel_id, topic);

    if (ipc_entity_manager_->hasSubscriber(topic)) {
      ipc_entity_manager_->removeSubscriber(topic);
    }
  }

  // Clean up WS Server side
  {
    state_.removeWsChannelToClientMapping(channel_id);

    ws_server_->removeChannels({ channel_id });
  }
  heph::log(heph::INFO, "[WS Bridge] - Topic dropped successfully.", "topic", topic);
}

void WebsocketBridge::callback_IpcGraph_ServiceFound(const std::string& service_name,
                                                     const heph::serdes::ServiceTypeInfo& type_info) {
  heph::log(heph::INFO, "[WS Bridge] - Service will be added  ...", "service_name", service_name,
            "request_type_name", type_info.request.name, "reply_type_name", type_info.reply.name);

  const WsServiceInfo new_ws_server_service = {
    .name = service_name,
    // This interface was built with the ROS2 convention in mind, that the request and reply types are two
    // pieces of a common type, hence the type name is the same with a _Request or _Reply suffix. This is not
    // the case for hephaestus. So we just chose the request type name.
    .type = type_info.request.name,

    .request = std::make_optional<WsServiceRequestDefinition>(
        convertSerializationTypeToString(type_info.request.serialization), type_info.request.name,
        convertSerializationTypeToString(type_info.request.serialization),
        convertProtoBytesToFoxgloveBase64String(type_info.request.schema)),

    .response = std::make_optional<WsServiceResponseDefinition>(
        convertSerializationTypeToString(type_info.reply.serialization), type_info.reply.name,
        convertSerializationTypeToString(type_info.reply.serialization),
        convertProtoBytesToFoxgloveBase64String(type_info.reply.schema)),

    // NOTE: These seem to be legacy fields that are not used (anymore) in the foxglove library.
    .requestSchema = std::nullopt,
    .responseSchema = std::nullopt,
  };

  const std::vector<WsServiceInfo> new_ws_server_services{ new_ws_server_service };
  auto new_service_ids = ws_server_->addServices(new_ws_server_services);

  CHECK_EQ(new_service_ids.size(), 1);
  const auto new_service_id = new_service_ids.front();

  state_.addWsServiceToIpcServiceMapping(new_service_id, service_name);

  heph::log(heph::INFO, "[WS Bridge] - Service was added successfully.", "service_name", service_name,
            "new_service_id", new_service_id);
}

void WebsocketBridge::callback_IpcGraph_ServiceDropped(const std::string& service_name) {
  heph::log(heph::INFO, "[WS Bridge] - Service will be dropped  ...", "service_name", service_name);

  if (!state_.hasIpcServiceMapping(service_name)) {
    if (config_.ws_server_verbose_bridge_state) {
      state_.printBridgeState();
    }
    heph::log(
        heph::WARN,
        fmt::format(
            "\n[WS Bridge] - Service is already unadvertized! There are likely multiple service servers!",
            "service_name", service_name));
    return;
  }

  const auto service_id = state_.getWsServiceForIpcService(service_name);
  state_.removeWsServiceToIpcServiceMapping(service_id, service_name);

  ws_server_->removeServices({ service_id });

  heph::log(heph::INFO, "[WS Bridge] - Service dropped successfully.", "service_name", service_name);
}

void WebsocketBridge::callback_IpcGraph_Updated(const ipc::zenoh::EndpointInfo& info,
                                                IpcGraphState ipc_graph_state) {
  // NOTE: currently we do not need the endpoint info that triggered the graph update. However, it is very
  // useful to debug, and there are also some features that might require knowing who triggered it. Let's keep
  // it.
  (void)info;

  if (config_.ws_server_verbose_ipc_graph_state) {
    ipc_graph_state.printIpcGraphState();
  }
  CHECK(ipc_graph_state.checkConsistency());

  if (config_.ws_server_verbose_bridge_state) {
    state_.printBridgeState();
  }
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
}

/////////////////////////////
// IPC Interface Callbacks //
/////////////////////////////

void WebsocketBridge::callback_Ipc_MessageReceived(const heph::ipc::zenoh::MessageMetadata& metadata,
                                                   std::span<const std::byte> message_data,
                                                   const heph::serdes::TypeInfo& type_info) {
  (void)type_info;

  CHECK(ws_server_);

  const std::string topic = metadata.topic;
  const WsChannelId channel_id = state_.getWsChannelForIpcTopic(topic);

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
                            // NOLINTNEXTLINE(bugprone-bitwise-pointer-cast)
                            std::bit_cast<const uint8_t*>(message_data.data()), message_data.size());
  }
}

void WebsocketBridge::callback_Ipc_ServiceResponsesReceived(
    WsServiceId service_id, WsServiceCallId call_id, const RawServiceResponses& responses,
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
                "\n[WS Bridge] - No client handle found for service call id. Dropping service respose.",
                "call_id", call_id, "service_id", service_id);
      return;
    }
    client_handle_w_name = client_handle_w_name_lookup.value();
    state_.removeCallIdToClientMapping(call_id);
  }

  const std::string client_name = client_handle_w_name.second;
  const auto client_handle = client_handle_w_name.first;

  if (responses.empty()) {
    auto msg = fmt::format("\n[WS Bridge] - Timeout - no service responses received.", "service_name",
                           service_name, "service_id", service_id);
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
    auto msg = fmt::format("\n[WS Bridge] - Response and request names do not "
                           "match!",
                           "service_name_in_response", response.topic, "service_name", service_name);

    heph::log(heph::ERROR, msg, "service_id", service_id, "call_id", call_id, "response_topic",
              response.topic, "service_name", service_name);
    ws_server_->sendServiceFailure(client_handle, service_id, call_id, msg);
    return;
  }

  WsServiceResponse ws_server_response;
  if (!convertIpcRawServiceResponseToWsServiceResponse(service_id, call_id, response, ws_server_response)) {
    auto msg = fmt::format("\n[WS Bridge] - Failed to convert IPC service response "
                           "to WS service response for service",
                           "service_name", service_name, "service_id", service_id);
    heph::log(heph::ERROR, msg, "service_id", service_id, "call_id", call_id, "service_name", service_name);
    ws_server_->sendServiceFailure(client_handle, service_id, call_id, msg);
    return;
  }

  ws_server_->sendServiceResponse(client_handle, ws_server_response);

  heph::log(heph::INFO, "[WS Bridge] - Client received service response successfully.", "client_name",
            client_name, "service_name", service_name, "service_id", service_id, "call_id", call_id,
            "sync_or_async", sync_service_call);
}

////////////////////////////////
// Websocket Server Callbacks //
////////////////////////////////

void WebsocketBridge::callback_Ws_Log(WsLogLevel level, const char* msg) {
  switch (level) {
    case WsLogLevel::Debug:
      heph::log(heph::DEBUG, fmt::format("\n[WS Server] - {}", msg));
      break;
    case WsLogLevel::Info:
      heph::log(heph::INFO, fmt::format("\n[WS Server] - {}", msg));
      break;
    case WsLogLevel::Warn:
      heph::log(heph::WARN, fmt::format("\n[WS Server] - {}", msg));
      break;
    case WsLogLevel::Error:
      heph::log(heph::ERROR, fmt::format("\n[WS Server] - {}", msg));
      break;
    case WsLogLevel::Critical:
      heph::log(heph::FATAL, fmt::format("\n[WS Server] - {}", msg));
      break;
  }
}

void WebsocketBridge::callback_Ws_Subscribe(WsChannelId channel_id, const WsClientHandle& client_handle) {
  CHECK(ipc_graph_);
  CHECK(ipc_entity_manager_);
  CHECK(ws_server_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const std::string topic = state_.getIpcTopicForWsChannel(channel_id);

  heph::log(heph::INFO, "[WS Bridge] - Client subscribes to topic...", "client_name", client_name, "topic",
            topic, "channel_id", channel_id);

  state_.addWsChannelToClientMapping(channel_id, client_handle, client_name);

  if (ipc_entity_manager_->hasSubscriber(topic)) {
    heph::log(heph::INFO, "[WS Bridge] - Client subcribed to topic successfully [IPC SUB EXISTS].",
              "client_name", client_name, "topic", topic, "channel_id", channel_id);
    return;
  }

  std::optional<heph::serdes::TypeInfo> topic_type_info = ipc_graph_->getTopicTypeInfo(topic);

  if (!topic_type_info.has_value()) {
    heph::log(heph::ERROR, "[WS Bridge] - Could not subscribe because failed to retrieve type", "topic",
              topic, "channel_id", channel_id);
    return;
  }

  ipc_entity_manager_->addSubscriber(topic, topic_type_info.value(),
                                     [this](const heph::ipc::zenoh::MessageMetadata& metadata,
                                            std::span<const std::byte> data,
                                            const heph::serdes::TypeInfo& type_info) {
                                       this->callback_Ipc_MessageReceived(metadata, data, type_info);
                                     });

  heph::log(heph::INFO, "[WS Bridge] - Client subcribed to topic successfully. [IPC SUB ADDED]",
            "client_name", client_name, "topic", topic, "channel_id", channel_id);
  if (config_.ws_server_verbose_bridge_state) {
    state_.printBridgeState();
  }
}

void WebsocketBridge::callback_Ws_Unsubscribe(WsChannelId channel_id, const WsClientHandle& client_handle) {
  CHECK(ipc_entity_manager_);
  CHECK(ws_server_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const std::string topic = state_.getIpcTopicForWsChannel(channel_id);

  heph::log(heph::INFO, "[WS Bridge] - Client unsubscribes from topic ...", "client_name", client_name,
            "topic", topic, "channel_id", channel_id);

  state_.removeWsChannelToClientMapping(channel_id, client_handle);

  if (!state_.hasWsChannelWithClients(channel_id)) {
    if (ipc_entity_manager_->hasSubscriber(topic)) {
      ipc_entity_manager_->removeSubscriber(topic);
      heph::log(heph::INFO, "[WS Bridge] - Client unsubscribed from topic successfully. [IPC SUB REMOVED]",
                "client_name", client_name, "topic", topic, "channel_id", channel_id);
    } else {
      heph::log(heph::INFO,
                "\n[WS Bridge] - Client unsubscribed from topic successfully. [IPC SUB NOT FOUND]",
                "client_name", client_name, "topic", topic, "channel_id", channel_id);
    }
  } else {
    heph::log(heph::INFO,
              "\n[WS Bridge] - Client unsubscribed from topic successfully. [IPC SUB STILL NEEDED]",
              "client_name", client_name, "topic", topic, "channel_id", channel_id);
  }
  if (config_.ws_server_verbose_bridge_state) {
    state_.printBridgeState();
  }
}

void WebsocketBridge::callback_Ws_ClientAdvertise(const WsClientChannelAd& advertisement,
                                                  const WsClientHandle& client_handle) {
  CHECK(ipc_graph_);
  CHECK(ipc_entity_manager_);
  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const auto& topic = advertisement.topic;
  const auto& channel_id = advertisement.channelId;

  heph::log(heph::INFO, "[WS Bridge] - Client advertises topic ...", "client_name", client_name, "topic",
            topic, "channel_id", channel_id);

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

  if (!ipc_entity_manager_->hasPublisher(topic)) {
    ipc_entity_manager_->addPublisher(topic, type_info.value());
    heph::log(heph::INFO, "[WS Bridge] - Client advertised topic successfully [IPC PUB ADDED]", "client_name",
              client_name, "topic", topic, "channel_id", channel_id);
  } else {
    heph::log(heph::INFO, "[WS Bridge] - Client advertised topic successfully [IPC PUB EXISTS]",
              "client_name", client_name, "topic", topic, "channel_id", channel_id);
  }

  if (config_.ws_server_verbose_bridge_state) {
    state_.printBridgeState();
  }
}

void WebsocketBridge::callback_Ws_ClientUnadvertise(WsClientChannelId client_channel_id,
                                                    const WsClientHandle& client_handle) {
  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  auto topic = state_.getTopicForClientChannel(client_channel_id);

  heph::log(heph::INFO, "[WS Bridge] - Client unadvertises topic ...", "client_name", client_name, "topic",
            topic, "client_channel_id", client_channel_id);

  if (topic.empty()) {
    heph::log(heph::ERROR, "[WS Bridge] - Client tried to unadvertise channel but the channel is unknown!",
              "client_name", client_name, "channel_id", client_channel_id);
    return;
  }

  auto client_handle_with_name = state_.getClientForClientChannel(client_channel_id);

  if (!client_handle_with_name.has_value()) {
    heph::log(
        heph::ERROR,
        "\n[WS Bridge] - Client tried to unadvertise topic but the channel is not owned by this client!",
        "client_name", client_name, "channel_id", client_channel_id, "topic", topic);
    return;
  }

  state_.removeClientChannelToTopicMapping(client_channel_id);
  state_.removeClientChannelToClientMapping(client_channel_id);

  if (!state_.hasClientChannelsForTopic(topic)) {
    if (ipc_entity_manager_->hasPublisher(topic)) {
      ipc_entity_manager_->removePublisher(topic);
      heph::log(heph::INFO, "[WS Bridge] - Client unadvertised topic successfully. [IPC PUB REMOVED]",
                "client_name", client_name, "topic", topic, "client_channel_id", client_channel_id);
    } else {
      heph::log(heph::INFO, "[WS Bridge] - Client unadvertised topic successfully. [IPC PUB NOT FOUND]",
                "client_name", client_name, "topic", topic, "client_channel_id", client_channel_id);
    }
  } else {
    heph::log(heph::INFO, "[WS Bridge] - Client unadvertised topic successfully. [IPC PUB STILL NEEDED]",
              "client_name", client_name, "topic", topic, "client_channel_id", client_channel_id);
  }

  if (config_.ws_server_verbose_bridge_state) {
    state_.printBridgeState();
  }
}

void WebsocketBridge::callback_Ws_ClientMessage(const WsClientMessage& message,
                                                const WsClientHandle& client_handle) {
  const std::string client_name = ws_server_->remoteEndpointString(client_handle);
  const auto& topic = message.advertisement.topic;
  const auto& channel_id = message.advertisement.channelId;

  if (!ipc_entity_manager_->hasPublisher(topic)) {
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
  constexpr size_t NUM_MESSAGE_HEADER_BYTES = 5;
  if (message.data.size() < NUM_MESSAGE_HEADER_BYTES) {
    heph::log(heph::ERROR, "[WS Bridge] - Client sent message with insufficient data!", "client_name",
              client_name, "topic", topic, "channel_id", channel_id, "message_size", message.data.size());
    return;
  }

  // 1 byte opcode + 4 bytes channel ID
  // The rest of the message is the payload
  const auto opcode = static_cast<uint8_t>(message.data[0]);

  auto channel_id_bytes = std::span<const uint8_t>(message.data).subspan(1, 4);
  const auto parsed_channel_id = foxglove::ReadUint32LE(channel_id_bytes.data());

  if (opcode != static_cast<uint8_t>(WsClientBinaryOpCode::MESSAGE_DATA)) {
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
  const auto payload_bytes = std::span<const uint8_t>(message.data).subspan(NUM_MESSAGE_HEADER_BYTES);
  const std::span<const std::byte> message_data = std::as_bytes(payload_bytes);

  if (!ipc_entity_manager_->publishMessage(topic, message_data)) {
    heph::log(heph::ERROR, "[WS Bridge] - Failed to publish client message!", "client_name", client_name,
              "topic", topic, "channel_id", channel_id);
  }
}

void WebsocketBridge::callback_Ws_ServiceRequest(const WsServiceRequest& request,
                                                 const WsClientHandle& client_handle) {
  CHECK(ipc_entity_manager_);

  const std::string client_name = ws_server_->remoteEndpointString(client_handle);

  if (!state_.hasWsServiceMapping(request.serviceId)) {
    auto msg =
        fmt::format("\n[WS Bridge] - Client '{}' sent service request for unadvertised service [{}/{}]!",
                    client_name, request.serviceId, request.callId);
    heph::log(heph::ERROR, msg, "service_id", request.serviceId, "call_id", request.callId, "client_name",
              client_name);
    ws_server_->sendServiceFailure(client_handle, request.serviceId, request.callId, msg);
    return;
  }

  if (request.encoding != "protobuf") {
    auto msg = fmt::format("\n[WS Bridge] - Client '{}' sent service request [{}/{}] with unsuported "
                           "encoding ({})!",
                           client_name, request.serviceId, request.callId, request.encoding);
    heph::log(heph::ERROR, msg, "service_id", request.serviceId, "call_id", request.callId, "client_name",
              client_name);
    ws_server_->sendServiceFailure(client_handle, request.serviceId, request.callId, msg);
    return;
  }

  auto service_name = state_.getIpcServiceForWsService(request.serviceId);
  const WsServiceId service_id = request.serviceId;
  const WsServiceCallId call_id = request.callId;
  heph::log(heph::INFO, "[WS Bridge] - Client sent service request ...", "client_name", client_name,
            "service_name", service_name, "service_id", service_id, "call_id", call_id);

  const ipc::TopicConfig topic_config(service_name);

  auto buffer = std::as_bytes(std::span(request.data.data(), request.data.size()));

  const std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(config_.ipc_service_call_timeout_ms);

  if (config_.ipc_service_service_request_async) {
    ///////////
    // ASYNC //
    ///////////

    state_.addCallIdToClientMapping(call_id, client_handle, client_name);

    auto response_callback = [this, service_id, call_id](const RawServiceResponses& responses) -> void {
      heph::log(heph::INFO, "[WS Bridge] - Service response callback triggered for service [ASYNC]",
                "num_responses", responses.size(), "service_id", service_id, "call_id", call_id);

      callback_Ipc_ServiceResponsesReceived(service_id, call_id, responses);
    };

    ipc_entity_manager_->callServiceAsync(call_id, topic_config, buffer, timeout_ms, response_callback);

    heph::log(heph::INFO, "[WS Bridge] - Client service request dispatched [ASYNC]", "client_name",
              client_name, "service_name", service_name, "service_id", service_id, "call_id", call_id);
  } else {
    //////////
    // SYNC //
    //////////
    auto responses = ipc_entity_manager_->callService(call_id, topic_config, buffer, timeout_ms);

    callback_Ipc_ServiceResponsesReceived(
        service_id, call_id, responses, std::make_optional<ClientHandleWithName>(client_handle, client_name));
  }
}

void WebsocketBridge::callback_Ws_SubscribeConnectionGraph(bool subscribe) {
  CHECK(ipc_graph_);
  if (subscribe) {
    ipc_graph_->refreshConnectionGraph();
  }
}

}  // namespace heph::ws
