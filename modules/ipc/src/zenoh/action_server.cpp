//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/action_server.h"

#include <string>

#include <absl/log/log.h>
#include <fmt/core.h>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/internal/action_server_client_helper.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {
auto requestActionServerToStopExecution(Session& session, const TopicConfig& topic_config) -> bool {
  static constexpr auto TIMEOUT = std::chrono::milliseconds{ 1000 };
  const auto stop_topic = internal::getStopServiceTopic(topic_config);
  auto results = callService<std::string, std::string>(session, TopicConfig{ stop_topic }, "", TIMEOUT);
  if (results.size() != 1) {
    LOG(ERROR) << fmt::format("Failed to stop the action server {}.", topic_config.name);
    return false;
  }

  return true;
}
}  // namespace heph::ipc::zenoh
