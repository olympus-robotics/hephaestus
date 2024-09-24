//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>

#include <fmt/core.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/action_server/types_protobuf.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"

namespace heph::ipc::zenoh::action_server::internal {
[[nodiscard]] auto getStatusPublisherTopic(const TopicConfig& server_topic) -> TopicConfig;

[[nodiscard]] auto getResponseServiceTopic(const TopicConfig& topic_config) -> TopicConfig;

[[nodiscard]] auto getStopServiceTopic(const TopicConfig& topic_config) -> TopicConfig;

template <typename ReplyT>
[[nodiscard]] auto handleFailure(const std::string& topic_name, const std::string& error_message,
                                 RequestStatus error) -> std::future<Response<ReplyT>> {
  LOG(ERROR) << fmt::format("Failed to call action server (topic: {}): {}.", topic_name, error_message);
  std::promise<Response<ReplyT>> promise;
  promise.set_value({ ReplyT{}, error });
  return promise.get_future();
}

/// If an action server is already serving a request it will reject the new request.
template <typename RequestT, typename StatusT, typename ReplyT>
class ClientHelper {
public:
  using StatusUpdateCallback = std::function<void(const StatusT&)>;
  ClientHelper(
      SessionPtr session, TopicConfig topic_config,
      StatusUpdateCallback&& status_update_cb);  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

  [[nodiscard]] auto getResponse() -> std::future<Response<ReplyT>>;

private:
  [[nodiscard]] auto serviceCallback(const Response<ReplyT>& reply) -> RequestResponse;

private:
  SessionPtr session_;
  TopicConfig topic_config_;

  std::unique_ptr<Subscriber<StatusT>> status_subscriber_;
  std::unique_ptr<Service<Response<ReplyT>, RequestResponse>> response_service_;

  std::promise<Response<ReplyT>> reply_promise_;
};

template <typename RequestT, typename StatusT, typename ReplyT>
ClientHelper<RequestT, StatusT, ReplyT>::ClientHelper(SessionPtr session, TopicConfig topic_config,
                                                      StatusUpdateCallback&& status_update_cb)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , status_subscriber_(createSubscriber<StatusT>(
        session_, internal::getStatusPublisherTopic(topic_config_),
        [status_update_cb = std::move(status_update_cb)](const MessageMetadata&,
                                                         const std::shared_ptr<StatusT>& status) mutable {
          status_update_cb(*status);
        }))
  , response_service_(std::make_unique<Service<Response<ReplyT>, RequestResponse>>(
        session_, internal::getResponseServiceTopic(topic_config_),
        [this](const Response<ReplyT>& reply) { return serviceCallback(reply); })) {
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ClientHelper<RequestT, StatusT, ReplyT>::getResponse() -> std::future<Response<ReplyT>> {
  return reply_promise_.get_future();
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ClientHelper<RequestT, StatusT, ReplyT>::serviceCallback(const Response<ReplyT>& reply)
    -> RequestResponse {
  reply_promise_.set_value(reply);
  return { .status = RequestStatus::SUCCESSFUL };
}

}  // namespace heph::ipc::zenoh::action_server::internal
