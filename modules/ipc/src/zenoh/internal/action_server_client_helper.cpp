//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/internal/action_server_client_helper.h"

#include <fmt/core.h>

#include "hephaestus/ipc/common.h"

namespace heph::ipc::zenoh::internal {
auto getStatusPublisherTopic(const TopicConfig& server_topic) -> TopicConfig {
  static constexpr auto STATUS_TOPIC_FORMAT = "{}/status_update";
  return { fmt::format(STATUS_TOPIC_FORMAT, server_topic.name) };
}

auto getResponseServiceTopic(const TopicConfig& topic_config) -> TopicConfig {
  static constexpr auto RESPONSE_TOPIC_FORMAT = "{}/response";
  return { fmt::format(RESPONSE_TOPIC_FORMAT, topic_config.name) };
}

auto getStopServiceTopic(const TopicConfig& topic_config) -> TopicConfig {
  static constexpr auto STOP_SERVICE_TOPIC_FORMAT = "{}/stop_request";
  return { fmt::format(STOP_SERVICE_TOPIC_FORMAT, topic_config.name) };
}
}  // namespace heph::ipc::zenoh::internal
