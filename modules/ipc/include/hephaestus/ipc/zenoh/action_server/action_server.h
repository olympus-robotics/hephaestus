//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/client_helper.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/action_server/types_proto.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/raw_publisher.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/log.h"

namespace heph::ipc::zenoh::action_server {

enum class TriggerStatus : uint8_t {
  SUCCESSFUL = 0,
  REJECTED = 1,
};

/// An action server is a server that execute a user function in response to trigger from a client.
/// Upon completion a result is sent back to the client.
/// Differently from classic request/response servers, action servers are asynchronous and non-blocking.
/// Action servers also provide functionalities for the user to send status updates to the client during
/// execution of the action and are interruptible.
/// To instantiate an ActionServer the user needs to provide two callbacks:
/// - TriggerCallback
///   - Takes as input the request and need to decide if the request is valid and if it can be served.
///   - No long running operations should be done in this callback.
/// - ExecuteCallback
///   - This is the function that does the actual work and eventually returns the final response to the
///   client.
///   - The execute callback is run in a dedicated thread.
///   - The function begins execution as soon as the request is accepted.
///   - The function has access to a Publisher to send status updates to the client.
///     - The frequency of the updates is decided by the user; updates are not mandatory
///   - The function can be interrupted by the client by setting the stop_requested flag to true.
///     - The function should check this flag periodically and return if it is set.
///     - The ability to stop the server relies on the user correctly reading the value of `stop_requested`.
///       If the user ignore the variable, the server cannot be stopped.
/// NOTES:
/// - If an action server is already serving a request it will not accept new ones.
/// - RequestT needs to be copyable and one copy will be made of it.
/// ---------------------------------------------------------------------------------------------------------
/// Implementation details:
/// - ActionServer contains a `Service` to receive the requests and a `MessageQueueConsumer` to execute them.
/// - When a new request is accepted and started, a new `Publisher` is created to send status updates.
/// - The status publisher is passed to `execute_cb`, which the user can use to publish status update of the
///   execution.
///   - When calling the server, the client will create a temporary `Subscriber` to receive the updates.
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
  using TriggerCallback = std::function<TriggerStatus(const RequestT&)>;
  using ExecuteCallback = std::function<ReplyT(const RequestT&, Publisher<StatusT>&, std::atomic_bool&)>;

  ActionServer(SessionPtr session, TopicConfig topic_config, TriggerCallback&& action_trigger_cb,
               ExecuteCallback&& execute_cb);
  ~ActionServer();
  ActionServer(const ActionServer&) = delete;
  ActionServer(ActionServer&&) = delete;
  auto operator=(const ActionServer&) -> ActionServer& = delete;
  auto operator=(ActionServer&&) -> ActionServer& = delete;

private:
  [[nodiscard]] auto onRequest(const Request<RequestT>& request) -> RequestResponse;
  void execute(const Request<RequestT>& request);

private:
  static constexpr auto REPLY_SERVICE_DEFAULT_TIMEOUT = std::chrono::milliseconds{ 1000 };

  SessionPtr session_;
  TopicConfig topic_config_;

  TriggerCallback action_trigger_cb_;
  ExecuteCallback execute_cb_;

  std::unique_ptr<Service<Request<RequestT>, RequestResponse>> request_service_;
  concurrency::MessageQueueConsumer<Request<RequestT>> request_consumer_;
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
                                    const RequestT& request, StatusUpdateCallback<StatusT>&& status_update_cb,
                                    std::chrono::milliseconds request_timeout)
    -> std::future<Response<ReplyT>>;

/// Request the action server to stop.
[[nodiscard]] auto requestActionServerToStopExecution(Session& session, const TopicConfig& topic_config)
    -> bool;

// TODO: add a function to get notified when the server is idle again.

// ----------------------------------------------------------------------------------------------------------
// --------- ActionServer Implementation --------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

template <typename RequestT, typename StatusT, typename ReplyT>
ActionServer<RequestT, StatusT, ReplyT>::ActionServer(SessionPtr session, TopicConfig topic_config,
                                                      TriggerCallback&& action_trigger_cb,
                                                      ExecuteCallback&& execute_cb)
  : session_(std::move((session)))
  , topic_config_(std::move(topic_config))
  , action_trigger_cb_(std::move(action_trigger_cb))
  , execute_cb_(std::move(execute_cb))
  , request_service_(std::make_unique<Service<Request<RequestT>, RequestResponse>>(
        session_, internal::getRequestServiceTopic(topic_config_),
        [this](const Request<RequestT>& request) { return onRequest(request); }, []() {}, []() {},
        ServiceConfig{
            .create_liveliness_token = false,
            .create_type_info_service = false,
        }))
  , request_consumer_([this](const Request<RequestT>& request) { return execute(request); }, std::nullopt) {
  request_consumer_.start();
  heph::log(heph::DEBUG, "started Action Server", "topic", topic_config_.name);
}

template <typename RequestT, typename StatusT, typename ReplyT>
ActionServer<RequestT, StatusT, ReplyT>::~ActionServer() {
  auto stopped = request_consumer_.stop();
  stopped.get();
}

template <typename RequestT, typename StatusT, typename ReplyT>
auto ActionServer<RequestT, StatusT, ReplyT>::onRequest(const Request<RequestT>& request) -> RequestResponse {
  if (is_running_.exchange(true)) {
    heph::log(heph::ERROR, "action server is already serving one request", "topic", topic_config_.name);

    return { .status = RequestStatus::REJECTED_ALREADY_RUNNING };
  }

  try {
    const auto response = action_trigger_cb_(request.request);
    if (response != TriggerStatus::SUCCESSFUL) {
      is_running_ = false;
      return { .status = RequestStatus::REJECTED_USER };
    }
  } catch (const std::exception& ex) {
    heph::log(heph::ERROR, "request callback failed", "topic", topic_config_.name, "exception", ex.what());
    is_running_ = false;
    return { .status = RequestStatus::INVALID };
  }

  if (!request_consumer_.queue().tryPush(request)) {
    // NOTE: this should never happen as the queue is unbound.
    heph::log(
        heph::ERROR,
        "failed to push the job in the queue. NOTE: this should not happen, something is wrong in the code!",
        "topic", topic_config_.name);
    return { .status = RequestStatus::INVALID };
  }

  heph::log(heph::DEBUG, "request accepted.", "topic", topic_config_.name);
  return { .status = RequestStatus::SUCCESSFUL };
}

template <typename RequestT, typename StatusT, typename ReplyT>
void ActionServer<RequestT, StatusT, ReplyT>::execute(const Request<RequestT>& request) {
  is_running_ = true;
  // NOTE: we create the publisher and the subscriber only if the request is successful.
  // This has the limit that that some status updates could be lost if the pub/sub are still discovering each
  // other, but has the great advantage that we do not risk receiving messages from other requests if our
  // request is rejected.
  PublisherConfig config;
  config.create_liveliness_token = false;
  config.create_type_info_service = false;
  auto status_update_publisher = std::make_unique<Publisher<StatusT>>(
      session_, internal::getStatusPublisherTopic(topic_config_, request.uid), nullptr, config);

  std::atomic_bool stop_requested{ false };  // NOLINT(misc-const-correctness) False positive
  auto stop_service = Service<std::string, std::string>(
      session_, internal::getStopServiceTopic(topic_config_),
      [&stop_requested](const std::string&) {
        stop_requested = true;
        return "stopped";
      },
      []() {}, []() {},
      {
          .create_liveliness_token = false,
          .create_type_info_service = false,
      });

  const auto reply = [this, session = session_, request,
                      status_update_publisher = std::move(status_update_publisher),
                      &stop_requested]() noexcept {
    try {
      return Response<ReplyT>{
        .value = execute_cb_(request.request, *status_update_publisher, stop_requested),
        .status = stop_requested ? RequestStatus::STOPPED : RequestStatus::SUCCESSFUL,
      };
    } catch (const std::exception& ex) {
      heph::log(heph::ERROR, "execute callback failed with exception", "topic", topic_config_.name,
                "exception", ex.what());
      return Response<ReplyT>{
        .value = ReplyT{},
        .status = RequestStatus::INVALID,
      };
    }
  }();

  auto response_topic = internal::getResponseServiceTopic(topic_config_, request.uid);

  const auto client_response = callService<Response<ReplyT>, RequestResponse>(
      *session_, response_topic, reply, REPLY_SERVICE_DEFAULT_TIMEOUT);
  if (client_response.size() != 1 || client_response.front().value.status != RequestStatus::SUCCESSFUL) {
    heph::log(heph::ERROR, "failed to send final response to client", "topic", topic_config_.name);
  }

  is_running_ = false;
}

// ----------------------------------------------------------------------------------------------------------
// --------- ActionServerClient Implementation --------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

template <typename RequestT, typename StatusT, typename ReplyT>
auto callActionServer(SessionPtr session, const TopicConfig& topic_config, const RequestT& request,
                      StatusUpdateCallback<StatusT>&& status_update_cb,
                      std::chrono::milliseconds request_timeout) -> std::future<Response<ReplyT>> {
  auto request_topic = internal::getRequestServiceTopic(topic_config);

  static constexpr std::size_t UID_SIZE = 10;
  auto mt = random::createRNG();
  const auto uid = random::random<std::string>(mt, UID_SIZE, false, true);

  auto client_helper = std::make_unique<internal::ClientHelper<RequestT, StatusT, ReplyT>>(
      session, topic_config, uid, std::move(status_update_cb));

  auto action_server_request = Request<RequestT>{
    .request = request,
    .uid = uid,
  };
  const auto server_responses = callService<Request<RequestT>, RequestResponse>(
      *session, request_topic, action_server_request, request_timeout);

  auto failure = internal::checkFailure<ReplyT>(server_responses, topic_config.name);
  if (failure.has_value()) {
    return std::move(*failure);
  }

  return std::async(std::launch::async, [client_helper = std::move(client_helper)]() mutable {
    auto response = client_helper->getResponse().get();
    return response;
  });
}

}  // namespace heph::ipc::zenoh::action_server
