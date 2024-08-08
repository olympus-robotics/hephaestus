//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <optional>

#include <magic_enum.hpp>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/publisher.h"
#include "hephaestus/ipc/zenoh/internal/action_server_client_helper.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/types/action_server_types.h"
#include "hephaestus/ipc/zenoh/types/action_server_types_protobuf.h"  // NOLINT(misc-include-cleaner)

namespace heph::ipc::zenoh {

enum class ActionServerTriggerStatus : uint8_t {
  SUCCESSFUL = 0,
  REJECTED = 1,
};

/// An action server is a server that execute a user function in response to trigger from a client.
/// Upon completion a result is sent back to the client.
/// Differently from classic request/response servers, action servers are asynchronous and non-blocking.
/// Action servers also provide functionalities for the user to send status updates to the client during
/// execution of the action and are interruptible.
/// To instantiate an ActionServer the user needs to provide two callbacks:
/// - ActionTriggerCallback
///   - Takes in input the request and need to decide if the request is valid and if it can be served.
///   - The callback should not block and return immediately,
/// - ExecuteCallback
///   - This is the function that does the actual work and eventually return the final response to the client.
///   - The execute callback is executed in a dedicated thread.
///   - The function begin execution as soon as the request is accepted.
///   - The function has access to a Publisher to send status updates to the client.
///     - The frequency of the updates is decided by the user; updates are not mandatory
///   - The function can be interrupted by the client by setting the stop_requested flag to true.
///     - The function should check this flag periodically and return if it is set.
/// NOTES:
/// - If an action server is already serving a request it will not accept new ones.
/// - RequestT needs to be copyable and one copy will be made of it.
/// ---------------------------------------------------------------------------------------------------------
/// Implementation details:
/// - ActionServer contains a `Service` to receive the requests and a `MessageQueueConsumer` to execute them.
/// - When a new request is accepted and started a new `Publisher` is created to send status updates.
/// - The publisher is passed to `execute_cb`, which the user can use to publish status update of the
///   execution.
///   - The client when calling the server will create a temporary `Subscriber` to receive the updates.
/// - When `execute_cb` finishes the final response is sent to the client via a `Service` created by the
///   caller.
/// NOTE: The reason why we can only process one request at a time is that with the current implementation the
/// response `Service` and the update `Publisher` uses a topic name which just depends on the input topic
/// name. This means that if we have multiple requests we will have multiple `Publisher` and `Subscriber` with
/// the same topic name, which will cause conflicts. This could be solved by adding a unique identifier to the
/// topic name, but this is not implemented yet.
template <typename RequestT, typename StatusT, typename ReplyT>
class ActionServer {
public:
  using ActionTriggerCallback = std::function<ActionServerTriggerStatus(const RequestT&)>;
  using ExecuteCallback =
      std::function<ReplyT(const RequestT&, Publisher<StatusT>&, std::atomic_bool& stop_requested)>;

  ActionServer(SessionPtr session, TopicConfig topic_config, ActionTriggerCallback&& action_trigger_cb,
               ExecuteCallback&& execute_cb);
  ~ActionServer();
  ActionServer(const ActionServer&) = delete;
  ActionServer(ActionServer&&) = delete;
  auto operator=(const ActionServer&) -> ActionServer& = delete;
  auto operator=(ActionServer&&) -> ActionServer& = delete;

private:
  [[nodiscard]] auto onRequest(const RequestT& request) -> ActionServerRequestResponse;
  void execute(const RequestT& request);

private:
  SessionPtr session_;
  TopicConfig topic_config_;

  ActionTriggerCallback action_trigger_cb_;
  ExecuteCallback execute_cb_;

  std::unique_ptr<Service<RequestT, ActionServerRequestResponse>> request_service_;
  concurrency::MessageQueueConsumer<RequestT> request_consumer_;
  std::atomic_bool is_running_{ false };
};

template <typename StatusT>
using StatusUpdateCallback = std::function<void(const StatusT&)>;

/// Call the action server with the given request.
/// - The status_update_cb will be called with the status updates from the server.
///   - The updates are decided by the user implementation of the action server.
/// - Returns a future which will eventually contain the response from the server.
template <typename RequestT, typename StatusT, typename ReplyT>
[[nodiscard]] auto callActionServer(SessionPtr session, const TopicConfig& topic_config,
                                    const RequestT& request, StatusUpdateCallback<StatusT>&& status_update_cb)
    -> std::future<ActionServerResponse<ReplyT>>;

/// Request the action server to stop.
[[nodiscard]] auto requestActionServerToStopExecution(Session& session,
                                                      const TopicConfig& topic_config) -> bool;

// TODO: add a function to get notified when the server is idle again.

// ----------------------------------------------------------------------------------------------------------
// --------- ActionServer Implementation --------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

template <typename RequestT, typename StatusT, typename ReplyT>
ActionServer<RequestT, StatusT, ReplyT>::ActionServer(SessionPtr session, TopicConfig topic_config,
                                                      ActionTriggerCallback&& action_trigger_cb,
                                                      ExecuteCallback&& execute_cb)
  : session_(std::move((session)))
  , topic_config_(std::move(topic_config))
  , action_trigger_cb_(std::move(action_trigger_cb))
  , execute_cb_(std::move(execute_cb))
  , request_service_(std::make_unique<Service<RequestT, ActionServerRequestResponse>>(
        session_, topic_config_, [this](const RequestT& request) { return onRequest(request); }))
  , request_consumer_([this](const RequestT& request) { return execute(request); }, std::nullopt) {
  request_consumer_.start();
}

template <typename RequestT, typename StatusT, typename ReplyT>
ActionServer<RequestT, StatusT, ReplyT>::~ActionServer() {
  request_consumer_.stop();
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ActionServer<RequestT, StatusT, ReplyT>::onRequest(const RequestT& request)
    -> ActionServerRequestResponse {
  if (is_running_) {
    LOG(ERROR) << fmt::format("ActionServer (topic: {}): server is already serving one request.",
                              topic_config_.name);

    return { .status = ActionServerRequestStatus::REJECTED_ALREADY_RUNNING };
  }

  try {
    const auto response = action_trigger_cb_(request);
    if (response != ActionServerTriggerStatus::SUCCESSFUL) {
      return { .status = ActionServerRequestStatus::REJECTED_USER };
    }
  } catch (const std::exception& ex) {
    LOG(ERROR) << fmt::format("ActionServer (topic: {}): request callback failed with exception: {}.",
                              topic_config_.name, ex.what());
    return { .status = ActionServerRequestStatus::INVALID };
  }

  if (!request_consumer_.queue().tryPush(request)) {
    // NOTE: this should never happen as the queue is unbound.
    LOG(ERROR) << fmt::format("ActionServer (topic: {}): failed to push the job in the queue. THIS SHOULD "
                              "NOT HAPPEN CHECK THE CODE.",
                              topic_config_.name);
    return { .status = ActionServerRequestStatus::REJECTED_ALREADY_RUNNING };
  }

  LOG(INFO) << fmt::format("ActionServer (topic: {}): request accepted.", topic_config_.name);
  return { .status = ActionServerRequestStatus::SUCCESSFUL };
}

template <typename RequestT, typename StatusT, typename ReplyT>
void ActionServer<RequestT, StatusT, ReplyT>::execute(const RequestT& request) {
  is_running_ = true;
  // NOTE: we create the publisher and the subscriber only if the request is successful.
  // This has the limit that that some status updates could be lost if the pub/sub are still discovering each
  // other, but has the great advantage that we do not risk receiving messages from other requests if our
  // request is rejected. auto publisher = Publisher<StatusT>{ session_,
  // internal::getStatusPublisherTopic(topic_config_) };
  auto status_update_publisher =
      Publisher<StatusT>{ session_, internal::getStatusPublisherTopic(topic_config_) };

  std::atomic_bool stop_requested{ false };
  auto stop_service = Service<std::string, std::string>(
      session_, internal::getStopServiceTopic(topic_config_), [&stop_requested](const std::string&) {
        stop_requested = true;
        return "stopped";
      });

  const auto reply = [this, &request, &status_update_publisher, &stop_requested]() noexcept {
    try {
      return ActionServerResponse<ReplyT>{
        .value = execute_cb_(request, status_update_publisher, stop_requested),
        .status = stop_requested ? ActionServerRequestStatus::STOPPED : ActionServerRequestStatus::SUCCESSFUL,
      };
    } catch (const std::exception& ex) {
      LOG(ERROR) << fmt::format("ActionServer (topic: {}): execute callback failed with exception: {}.",
                                topic_config_.name, ex.what());
      return ActionServerResponse<ReplyT>{};
    }
  }();
  const auto response_topic = internal::getResponseServiceTopic(topic_config_);
  const auto client_response = callService<ActionServerResponse<ReplyT>, ActionServerRequestResponse>(
      *session_, response_topic, reply);
  if (client_response.size() != 1 ||
      client_response.front().value.status != ActionServerRequestStatus::SUCCESSFUL) {
    LOG(ERROR) << fmt::format("ActionServer (topic: {}): failed to send final response to client.",
                              topic_config_.name);
  }

  is_running_ = false;
}

// ----------------------------------------------------------------------------------------------------------
// --------- ActionServerClient Implementation --------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

template <typename RequestT, typename StatusT, typename ReplyT>
auto callActionServer(SessionPtr session, const TopicConfig& topic_config, const RequestT& request,
                      StatusUpdateCallback<StatusT>&& status_update_cb)
    -> std::future<ActionServerResponse<ReplyT>> {
  static constexpr auto TIMEOUT = std::chrono::milliseconds{ 1000 };
  const auto server_responses =
      callService<RequestT, ActionServerRequestResponse>(*session, topic_config, request, TIMEOUT);
  if (server_responses.empty()) {
    return internal::handleFailure<ReplyT>(topic_config.name, "no response",
                                           ActionServerRequestStatus::INVALID);
  }

  if (server_responses.size() > 1) {
    return internal::handleFailure<ReplyT>(
        topic_config.name,
        fmt::format(
            "received more than one response ({}), make sure the topic matches a single action server",
            server_responses.size()),
        ActionServerRequestStatus::INVALID);
  }

  const auto& server_response_status = server_responses.front().value.status;
  if (server_response_status != ActionServerRequestStatus::SUCCESSFUL) {
    return internal::handleFailure<ReplyT>(
        topic_config.name, fmt::format("request rejected {}", magic_enum::enum_name(server_response_status)),
        server_response_status);
  }

  return std::async(
      // NOLINTNEXTLINE(bugprone-exception-escape)
      std::launch::async, [session, topic_config, status_update_cb = std::move(status_update_cb)]() mutable {
        auto client_helper =
            internal::ActionServerClientHelper<RequestT, StatusT, ReplyT>{ session, topic_config,
                                                                           std::move(status_update_cb) };

        return client_helper.getResponse().get();
      });
}

}  // namespace heph::ipc::zenoh
