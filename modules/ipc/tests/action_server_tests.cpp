//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::action_server::tests {
namespace {

constexpr auto SERVICE_CALL_TIMEOUT = std::chrono::milliseconds{ 10 };

using DummyActionServer = ActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>;
struct ActionServerData {
  TopicConfig topic_config;
  SessionPtr session;
  DummyActionServer action_server;
};

[[nodiscard]] auto createDummyActionServer(std::mt19937_64& mt,
                                           DummyActionServer::TriggerCallback&& trigger_cb,
                                           DummyActionServer::ExecuteCallback&& execute_cb)
    -> ActionServerData {
  static constexpr int TOPIC_LENGTH = 10;
  auto service_topic = ipc::TopicConfig(
      fmt::format("test_action_server/{}", random::random<std::string>(mt, TOPIC_LENGTH, false, true)));

  ipc::zenoh::Config server_config{};
  auto server_session = ipc::zenoh::createSession(std::move(server_config));

  return {
    .topic_config = service_topic,
    .session = server_session,
    .action_server = ActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
        server_session, service_topic, std::move(trigger_cb), std::move(execute_cb)),
  };
}

TEST(ActionServer, RejectedCall) {
  auto mt = random::createRNG();

  auto action_server_data = createDummyActionServer(
      mt,
      [](const types::DummyType& request) {
        (void)request;
        return TriggerStatus::REJECTED;
      },
      [&](const types::DummyType& request, Publisher<types::DummyPrimitivesType>& status_publisher,
          std::atomic_bool& stop_requested) {
        (void)status_publisher;
        (void)stop_requested;
        return request;
      });

  auto request = types::DummyType::random(mt);
  const auto reply = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
                         action_server_data.session, action_server_data.topic_config, request,
                         [](const types::DummyPrimitivesType& status) { (void)status; }, SERVICE_CALL_TIMEOUT)
                         .get();

  EXPECT_EQ(reply.status, RequestStatus::REJECTED_USER);
}

TEST(ActionServer, ActionServerSuccessfulCall) {
  auto mt = random::createRNG();

  auto status = types::DummyPrimitivesType::random(mt);
  auto action_server_data = createDummyActionServer(
      mt,
      [](const types::DummyType& request) {
        (void)request;
        return TriggerStatus::SUCCESSFUL;
      },
      [&status](const types::DummyType& request, Publisher<types::DummyPrimitivesType>& status_publisher,
                std::atomic_bool& stop_requested) {
        auto success = status_publisher.publish(status);
        EXPECT_TRUE(success);
        (void)stop_requested;
        return request;
      });

  auto request = types::DummyType::random(mt);
  types::DummyPrimitivesType received_status;
  const auto reply =
      callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
          action_server_data.session, action_server_data.topic_config, request,
          [&received_status](const types::DummyPrimitivesType& status) { received_status = status; })
          .get();

  EXPECT_EQ(status, received_status);
  EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
  EXPECT_EQ(reply.value, request);
}

TEST(ActionServer, ActionServerStopRequest) {
  auto mt = random::createRNG();

  std::atomic_flag requested_started = ATOMIC_FLAG_INIT;
  auto action_server_data = createDummyActionServer(
      mt,
      [](const types::DummyType& request) {
        (void)request;
        return TriggerStatus::SUCCESSFUL;
      },
      [&requested_started](const types::DummyType& request,
                           Publisher<types::DummyPrimitivesType>& status_publisher,
                           std::atomic_bool& stop_requested) {
        (void)status_publisher;
        requested_started.test_and_set();
        requested_started.notify_all();

        while (!stop_requested) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return request;
      });

  auto request = types::DummyType::random(mt);
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request,
      [](const types::DummyPrimitivesType& status) { (void)status; }, SERVICE_CALL_TIMEOUT);

  requested_started.wait(false);
  auto success =
      requestActionServerToStopExecution(*action_server_data.session, action_server_data.topic_config);
  EXPECT_TRUE(success);

  const auto reply = reply_future.get();

  EXPECT_EQ(reply.status, RequestStatus::STOPPED);
  EXPECT_EQ(reply.value, request);
}

TEST(ActionServer, ActionServerRejectedAlreadyRunning) {
  auto mt = random::createRNG();

  std::atomic_flag requested_started = ATOMIC_FLAG_INIT;
  std::atomic_flag stop = ATOMIC_FLAG_INIT;
  auto action_server_data = createDummyActionServer(
      mt,
      [](const types::DummyType& request) {
        (void)request;
        return TriggerStatus::SUCCESSFUL;
      },
      [&requested_started, &stop](const types::DummyType& request,
                                  Publisher<types::DummyPrimitivesType>& status_publisher,
                                  std::atomic_bool& stop_requested) {
        (void)status_publisher;
        (void)stop_requested;
        requested_started.test_and_set();
        requested_started.notify_all();

        stop.wait(false);

        return request;
      });

  auto request = types::DummyType::random(mt);
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request,
      [](const types::DummyPrimitivesType& status) { (void)status; }, SERVICE_CALL_TIMEOUT);

  auto other_reply = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
                         action_server_data.session, action_server_data.topic_config, request,
                         [](const types::DummyPrimitivesType& status) { (void)status; }, SERVICE_CALL_TIMEOUT)
                         .get();
  EXPECT_EQ(other_reply.status, RequestStatus::REJECTED_ALREADY_RUNNING);

  // Stop the original request
  stop.test_and_set();
  stop.notify_all();
  reply_future.get();
}
}  // namespace
}  // namespace heph::ipc::zenoh::action_server::tests
