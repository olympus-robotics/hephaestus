//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/action_server/action_server.h"

#include <chrono>
#include <string>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/client_helper.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"

namespace heph::ipc::zenoh::action_server {
auto requestActionServerToStopExecution(Session& session, const TopicConfig& topic_config) -> bool {
  static constexpr auto TIMEOUT = std::chrono::milliseconds{ 1000 };
  const auto stop_topic = internal::getStopServiceTopic(topic_config);
  auto results = callService<std::string, std::string>(session, TopicConfig{ stop_topic }, "", TIMEOUT);
  if (results.size() != 1) {
    heph::log(heph::ERROR, "failed to stop the action server", "topic", topic_config.name);
    return false;
  }

  return true;
}
}  // namespace heph::ipc::zenoh::action_server
