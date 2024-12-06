//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <optional>
#include <utility>
#include <vector>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

template <typename RequestT, typename ReplyT>
class ServiceClient {
public:
  ServiceClient(SessionPtr session, TopicConfig topic_config);
  auto call(const RequestT& request, const std::optional<std::chrono::milliseconds>& timeout = std::nullopt)
      -> std::vector<ServiceResponse<ReplyT>>;

private:
  SessionPtr session_;
  TopicConfig topic_config_;
};

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <typename RequestT, typename ReplyT>
ServiceClient<RequestT, ReplyT>::ServiceClient(SessionPtr session, TopicConfig topic_config)
  : session_(std::move(session)), topic_config_(std::move(topic_config)) {
}

template <typename RequestT, typename ReplyT>
auto ServiceClient<RequestT, ReplyT>::call(
    const RequestT& request, const std::optional<std::chrono::milliseconds>& timeout /*= std::nullopt*/)
    -> std::vector<ServiceResponse<ReplyT>> {
  return callService(*session_, topic_config_, request, timeout);
}

}  // namespace heph::ipc::zenoh
