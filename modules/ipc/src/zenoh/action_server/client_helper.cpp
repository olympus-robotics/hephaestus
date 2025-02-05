//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/action_server/client_helper.h"

#include <fmt/format.h>

#include "hephaestus/ipc/topic.h"

namespace heph::ipc::zenoh::action_server::internal {
auto getStatusPublisherTopic(const TopicConfig& server_topic) -> TopicConfig {
  static constexpr auto STATUS_TOPIC_FORMAT = "{}/status_update";
  return TopicConfig{ fmt::format(STATUS_TOPIC_FORMAT, server_topic.name) };
}

auto getResponseServiceTopic(const TopicConfig& topic_config) -> TopicConfig {
  static constexpr auto RESPONSE_TOPIC_FORMAT = "{}/response";
  return TopicConfig{ fmt::format(RESPONSE_TOPIC_FORMAT, topic_config.name) };
}

auto getStopServiceTopic(const TopicConfig& topic_config) -> TopicConfig {
  static constexpr auto STOP_SERVICE_TOPIC_FORMAT = "{}/stop_request";
  return TopicConfig{ fmt::format(STOP_SERVICE_TOPIC_FORMAT, topic_config.name) };
}
}  // namespace heph::ipc::zenoh::action_server::internal
