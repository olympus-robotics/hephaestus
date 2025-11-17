//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/action_server/types_proto.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/telemetry/log/log.h"

namespace heph::ipc::zenoh::action_server::internal {
[[nodiscard]] auto getActionServerInternalTopicPrefix() -> std::string;

[[nodiscard]] auto getRequestServiceTopic(const TopicConfig& server_topic) -> TopicConfig;

[[nodiscard]] auto getStatusPublisherTopic(const TopicConfig& server_topic, std::string_view uid)
    -> TopicConfig;

[[nodiscard]] auto getResponseServiceTopic(const TopicConfig& topic_config, std::string_view uid)
    -> TopicConfig;

[[nodiscard]] auto getStopServiceTopic(const TopicConfig& topic_config) -> TopicConfig;

template <typename ReplyT>
[[nodiscard]] auto handleFailure(const std::string& topic_name, const std::string& error_message,
                                 RequestStatus error) -> std::future<Response<ReplyT>> {
  heph::log(heph::ERROR, "failed to call action server ", "topic", topic_name, "error", error_message);
  std::promise<Response<ReplyT>> promise;
  promise.set_value({ ReplyT{}, error });
  return promise.get_future();
}

template <typename ReplyT>
[[nodiscard]] auto checkFailure(const std::vector<ServiceResponse<RequestResponse>>& responses,
                                const std::string& topic_name)
    -> std::optional<std::future<Response<ReplyT>>> {
  if (responses.empty()) {
    return internal::handleFailure<ReplyT>(topic_name, "no response", RequestStatus::INVALID);
  }

  if (responses.size() > 1) {
    return internal::handleFailure<ReplyT>(
        topic_name,
        fmt::format(
            "received more than one response ({}), make sure the topic matches a single action server",
            responses.size()),
        RequestStatus::INVALID);
  }

  const auto& server_response_status = responses.front().value.status;
  if (server_response_status != RequestStatus::SUCCESSFUL) {
    return internal::handleFailure<ReplyT>(
        topic_name, fmt::format("request rejected {}", magic_enum::enum_name(server_response_status)),
        server_response_status);
  }

  return std::nullopt;
}

/// If an action server is already serving a request it will reject the new request.
template <typename RequestT, typename StatusT, typename ReplyT>
class ClientHelper {
public:
  using StatusUpdateCallback = std::function<void(const StatusT&)>;
  ClientHelper(
      SessionPtr session, TopicConfig topic_config, std::string uid,
      StatusUpdateCallback&& status_update_cb);  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

  [[nodiscard]] auto getResponse() -> std::future<Response<ReplyT>>;

private:
  [[nodiscard]] auto serviceCallback(const Response<ReplyT>& reply) -> RequestResponse;
  void onFailure();
  void postReplyServiceCallback();

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  std::string uid_;

  // The reply and reply promise need to initialized before the response service
  // otherwise, a data race between initialization and the response callbacks might
  // occur
  Response<ReplyT> reply_;
  std::promise<Response<ReplyT>> reply_promise_;

  std::unique_ptr<Subscriber<StatusT>> status_subscriber_;
  std::unique_ptr<Service<Response<ReplyT>, RequestResponse>> response_service_;
};

template <typename RequestT, typename StatusT, typename ReplyT>
ClientHelper<RequestT, StatusT, ReplyT>::ClientHelper(SessionPtr session, TopicConfig topic_config,
                                                      std::string uid,
                                                      StatusUpdateCallback&& status_update_cb)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , uid_(std::move(uid))
  , status_subscriber_(createSubscriber<StatusT>(
        session_, internal::getStatusPublisherTopic(topic_config_, uid_),
        [status_update_cb = std::move(status_update_cb)](const MessageMetadata&,
                                                         const std::shared_ptr<StatusT>& status) mutable {
          status_update_cb(*status);
        },
        {
            .cache_size = std::nullopt,
            .dedicated_callback_thread = false,
            .create_liveliness_token = false,
            .create_type_info_service = false,
        }))
  , response_service_(std::make_unique<Service<Response<ReplyT>, RequestResponse>>(
        session_, internal::getResponseServiceTopic(topic_config_, uid_),
        [this](const Response<ReplyT>& reply) { return serviceCallback(reply); }, [this]() { onFailure(); },
        [this]() { postReplyServiceCallback(); },
        ServiceConfig{ .create_liveliness_token = false, .create_type_info_service = false })) {
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ClientHelper<RequestT, StatusT, ReplyT>::getResponse() -> std::future<Response<ReplyT>> {
  return reply_promise_.get_future();
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ClientHelper<RequestT, StatusT, ReplyT>::serviceCallback(const Response<ReplyT>& reply)
    -> RequestResponse {
  reply_ = reply;
  return { .status = RequestStatus::SUCCESSFUL };
}

template <typename RequestT, typename StatusT, typename ReplyT>
void ClientHelper<RequestT, StatusT, ReplyT>::onFailure() {
  reply_promise_.set_value(Response<ReplyT>{ ReplyT{}, RequestStatus::INVALID });
}

template <typename RequestT, typename StatusT, typename ReplyT>
void ClientHelper<RequestT, StatusT, ReplyT>::postReplyServiceCallback() {
  reply_promise_.set_value(std::move(reply_));
}

}  // namespace heph::ipc::zenoh::action_server::internal
