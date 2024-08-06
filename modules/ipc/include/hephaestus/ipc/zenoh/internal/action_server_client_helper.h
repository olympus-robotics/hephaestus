//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>

#include <fmt/core.h>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/subscriber.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/ipc/zenoh/types/action_server_types.h"
#include "hephaestus/ipc/zenoh/types/action_server_types_protobuf.h"

namespace heph::ipc::zenoh::internal {
[[nodiscard]] auto getStatusPublisherTopic(const TopicConfig& server_topic) -> TopicConfig;

[[nodiscard]] auto getResponseServiceTopic(const TopicConfig& topic_config) -> TopicConfig;

[[nodiscard]] auto getStopServiceTopic(const TopicConfig& topic_config) -> TopicConfig;

/// If an action server is already serving a request it will reject the new request.
template <typename RequestT, typename StatusT, typename ReplyT>
class ActionServerClientHelper {
public:
  using StatusUpdateCallback = std::function<void(const StatusT&)>;
  ActionServerClientHelper(
      SessionPtr session, TopicConfig topic_config,
      StatusUpdateCallback&& status_update_cb);  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

  [[nodiscard]] auto getResponse() -> std::future<ReplyT>;

private:
  [[nodiscard]] auto serviceCallback(const ReplyT& reply) -> ActionServerRequestResponse;

private:
  SessionPtr session_;
  TopicConfig topic_config_;

  std::unique_ptr<Subscriber> status_subscriber_;
  std::unique_ptr<Service<ReplyT, ActionServerRequestResponse>> response_service_;

  std::promise<ReplyT> reply_promise_;
};

template <typename RequestT, typename StatusT, typename ReplyT>
ActionServerClientHelper<RequestT, StatusT, ReplyT>::ActionServerClientHelper(
    SessionPtr session, TopicConfig topic_config, StatusUpdateCallback&& status_update_cb)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , status_subscriber_(subscribe<Subscriber, StatusT>(
        session_, internal::getStatusPublisherTopic(topic_config_),
        [status_update_cb = std::move(status_update_cb)](const MessageMetadata&,
                                                         const std::shared_ptr<StatusT>& status) mutable {
          status_update_cb(*status);
        }))
  , response_service_(std::make_unique<Service<ReplyT, ActionServerRequestResponse>>(
        session_, internal::getResponseServiceTopic(topic_config_),
        [this](const ReplyT& reply) { return serviceCallback(reply); })) {
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ActionServerClientHelper<RequestT, StatusT, ReplyT>::getResponse() -> std::future<ReplyT> {
  return reply_promise_.get_future();
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ActionServerClientHelper<RequestT, StatusT, ReplyT>::serviceCallback(const ReplyT& reply)
    -> ActionServerRequestResponse {
  reply_promise_.set_value(reply);
  return { .status = ActionServerRequestStatus::ACCEPTED };
}

}  // namespace heph::ipc::zenoh::internal
