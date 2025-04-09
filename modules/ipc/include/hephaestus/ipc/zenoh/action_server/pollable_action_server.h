//================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>

#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh::action_server {

/// A wrapper around ActionServer which facilitates the implementation of action servers using the polling
/// paradigm.
///
/// The code implementing the action server is expected to call 'pollRequest' at a sufficiently high frequency
/// to check for new requests. When a new request is returned, then it's expected that the calling code takes
/// care of executing the action and calls 'complete' when the action is completed.
///
/// The action server will not accept new requests if an action is still in progress.
template <typename RequestT, typename StatusT, typename ReplyT>
class PollableActionServer {
public:
  /// Constructs a new action server.
  PollableActionServer(SessionPtr session, TopicConfig topic_config)
    : action_server_(
          std::move(session), std::move(topic_config),
          [this](const auto& command) {
            std::unique_lock lock(mutex_);
            if (state_ != State::IDLE) {
              heph::log(
                  heph::ERROR,
                  "Can't start an action when a previous action on the same topic is still in progress.");
              return TriggerStatus::REJECTED;
            }

            return TriggerStatus::SUCCESSFUL;
          },
          [this](const RequestT& request, Publisher<StatusT>& status_publisher,
                 std::atomic_bool& stop_requested) -> ReplyT {
            std::unique_lock lock(mutex_);

            state_ = State::REQUEST_PENDING;
            request_ = request;

            while (state_ != State::COMPLETED) {
              static constexpr std::chrono::milliseconds STOP_REQUESTED_POLL_RATE{ 50 };
              condition_variable_.wait_for(lock, STOP_REQUESTED_POLL_RATE);

              if (stop_requested && state_ == State::IN_PROGRESS) {
                state_ = State::IN_PROGRESS_SHOULD_ABORT;
              }

              if (status_.has_value()) {
                status_publisher.publish(*std::move(status_));
              }
            }

            state_ = State::IDLE;

            return *std::move(reply_);
          }) {
  }

  /// If a new request is pending, returns the pending request and switches the PollingActionServer to the
  /// IN_PROGRESS state. The code implementing the action server is expecting to execute the action and call
  /// 'complete' when the action is completed.
  ///
  /// In all other cases, the return value is std::nullopt.
  ///
  /// Note: It is allowed to call this function when a action is in progress, though the return value will be
  /// std::nullopt, not the request which started the action.
  auto pollRequest() -> std::optional<RequestT> {
    std::unique_lock lock(mutex_);

    if (state_ != State::REQUEST_PENDING) {
      return std::nullopt;
    }

    state_ = State::IN_PROGRESS;

    return std::move(request_);
  }

  /// Completes the currently running action with the given reply.
  ///
  /// This function should only be called when an action is currently in progress (which includes the case
  /// when it's being aborted).
  void complete(ReplyT reply) {
    std::unique_lock lock(mutex_);

    heph::panicIf(state_ != State::IN_PROGRESS && state_ != State::IN_PROGRESS_SHOULD_ABORT,
                  "PollableActionServer::complete should only be called when the current state is "
                  "IN_PROGRESS or IN_PROGRESS_SHOULD_ABORT.");

    reply_ = std::move(reply);
    state_ = State::COMPLETED;

    condition_variable_.notify_one();
  }

  /// Sets the action server status.
  ///
  /// This function should only be called when an action is currently in progress (which includes the case
  /// when it's being aborted).
  void setStatus(StatusT status) {
    std::unique_lock lock(mutex_);

    heph::panicIf(state_ != State::IN_PROGRESS && state_ != State::IN_PROGRESS_SHOULD_ABORT,
                  "PollableActionServer::setStatus should only be called when the current state is "
                  "IN_PROGRESS or IN_PROGRESS_SHOULD_ABORT.");

    status_ = status;
    condition_variable_.notify_one();
  }

  /// Returns true if the current action should be aborted.
  ///
  /// It's expected (though not mandatory) that the code implementing the action server aborts its current
  /// action as fast as possible and then calls 'complete'.
  auto shouldAbort() -> bool {
    std::unique_lock lock(mutex_);
    return state_ == State::IN_PROGRESS_SHOULD_ABORT;
  }

private:
  ActionServer<RequestT, StatusT, ReplyT> action_server_;

  std::mutex mutex_;
  std::condition_variable condition_variable_;

  enum class State {
    IDLE,
    REQUEST_PENDING,
    IN_PROGRESS,
    IN_PROGRESS_SHOULD_ABORT,
    COMPLETED,
  };

  State state_{ State::IDLE };
  std::optional<RequestT> request_;
  std::optional<StatusT> status_;
  std::optional<ReplyT> reply_;
};

}  // namespace heph::ipc::zenoh::action_server
