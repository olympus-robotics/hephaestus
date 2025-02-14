//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/action_server/client_helper.h"
#include "hephaestus/ipc/zenoh/action_server/types_proto.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/log.h"

namespace heph::ipc::zenoh::action_server {

template <typename RequestT, typename StatusT, typename ReplyT>
class ActionServerClient {
public:
  ActionServerClient(SessionPtr session, TopicConfig topic_config,
                     StatusUpdateCallback<StatusT>&& status_update_cb,
                     std::chrono::milliseconds request_timeout);

  [[nodiscard]] auto call(const RequestT& request) -> std::future<Response<ReplyT>>;

private:
  [[nodiscard]] auto serviceCallback(const Response<ReplyT>& reply) -> RequestResponse;
  void onFailure();
  void postReplyServiceCallback();

private:
  static constexpr std::size_t UID_SIZE = 10;

  SessionPtr session_;
  TopicConfig topic_config_;
  std::mt19937_64 mt_;
  std::string uid_;

  TopicConfig service_request_topic_;
  TopicConfig service_response_topic_;

  Response<ReplyT> reply_;
  std::promise<Response<ReplyT>> reply_promise_;

  std::unique_ptr<Subscriber<StatusT>> status_subscriber_;
  std::unique_ptr<Service<Response<ReplyT>, RequestResponse>> response_service_;
  std::chrono::milliseconds request_timeout_;

  std::atomic_bool is_running_{ false };
};

template <typename RequestT, typename StatusT, typename ReplyT>
ActionServerClient<RequestT, StatusT, ReplyT>::ActionServerClient(
    SessionPtr session, TopicConfig topic_config, StatusUpdateCallback<StatusT>&& status_update_cb,
    std::chrono::milliseconds request_timeout)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , mt_(random::createRNG())
  , uid_(random::random<std::string>(mt_, UID_SIZE, false, true))
  , service_request_topic_(internal::getRequestServiceTopic(topic_config_))
  , service_response_topic_(internal::getResponseServiceTopic(topic_config_, uid_))
  , status_subscriber_(createSubscriber<StatusT>(
        session_, internal::getStatusPublisherTopic(topic_config_),
        [status_update_cb = std::move(status_update_cb)](const MessageMetadata&,
                                                         const std::shared_ptr<StatusT>& status) mutable {
          status_update_cb(*status);
        },
        { .cache_size = std::nullopt,
          .create_type_info_service = false,
          .create_liveliness_token = false,
          .dedicated_callback_thread = false }))
  , response_service_(std::make_unique<Service<Response<ReplyT>, RequestResponse>>(
        session_, internal::getResponseServiceTopic(topic_config_, uid_),
        [this](const Response<ReplyT>& reply) { return serviceCallback(reply); }, [this]() { onFailure(); },
        [this]() { postReplyServiceCallback(); },
        ServiceConfig{ .create_liveliness_token = false, .create_type_info_service = false }))
  , request_timeout_(request_timeout) {
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ActionServerClient<RequestT, StatusT, ReplyT>::call(const RequestT& request)
    -> std::future<Response<ReplyT>> {
  if (is_running_.exchange(true)) {
    heph::log(heph::WARN, "action server client has already made a call and it's waiting on the response ",
              "topic", topic_config_.name);
    return internal::handleFailure<ReplyT>(topic_config_.name, "already running",
                                           RequestStatus::REJECTED_ALREADY_RUNNING);
  }

  reply_promise_ = {};  // Reset the promise

  auto action_server_request =
      Request<RequestT>{ .request = request, .response_service_topic = service_response_topic_.name };
  const auto server_responses = callService<Request<RequestT>, RequestResponse>(
      *session_, service_request_topic_, action_server_request, request_timeout_);

  auto failure = internal::checkFailure<ReplyT>(server_responses, topic_config_.name);
  if (failure.has_value()) {
    return std::move(*failure);
  }

  return std::async(std::launch::async, [this]() {
    // auto result = client_helper_.getResponse().get();
    auto result = reply_promise_.get_future().get();
    heph::log(heph::DEBUG, "received response from client helper, returning");
    is_running_ = false;
    return result;
  });
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ActionServerClient<RequestT, StatusT, ReplyT>::serviceCallback(const Response<ReplyT>& reply)
    -> RequestResponse {
  heph::log(heph::WARN, "Received response from action server -> return successful to the server");
  // reply_promise_.set_value(reply);
  reply_ = reply;
  return { .status = RequestStatus::SUCCESSFUL };
}

template <typename RequestT, typename StatusT, typename ReplyT>
void ActionServerClient<RequestT, StatusT, ReplyT>::onFailure() {
  heph::log(heph::WARN, "Post reply service callback - FAILURE!!!!");
  reply_promise_.set_value({ .value = {}, .status = RequestStatus::INVALID });
}

template <typename RequestT, typename StatusT, typename ReplyT>
void ActionServerClient<RequestT, StatusT, ReplyT>::postReplyServiceCallback() {
  heph::log(heph::WARN, "Post reply service callback - setting promise value");
  try {
    reply_promise_.set_value(std::move(reply_));
  } catch (std::exception& e) {
    heph::log(heph::ERROR, "Failed to set promise value", "exception", e.what());
  }
  heph::log(heph::WARN, "Post reply service callback - promise value SET");
}

}  // namespace heph::ipc::zenoh::action_server
