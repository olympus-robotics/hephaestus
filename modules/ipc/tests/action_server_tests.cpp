//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
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

  Config server_config{};
  auto server_session = createSession(std::move(server_config));

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
      mt, [](const types::DummyType&) { return TriggerStatus::REJECTED; },
      [&](const types::DummyType& request, Publisher<types::DummyPrimitivesType>&, std::atomic_bool&) {
        return request;
      });

  auto request = types::DummyType::random(mt);
  const auto reply = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
                         action_server_data.session, action_server_data.topic_config, request,
                         [](const types::DummyPrimitivesType&) {}, SERVICE_CALL_TIMEOUT)
                         .get();

  EXPECT_EQ(reply.status, RequestStatus::REJECTED_USER);
}

TEST(ActionServer, ActionServerSuccessfulCall) {
  auto mt = random::createRNG();

  auto status = types::DummyPrimitivesType::random(mt);
  auto action_server_data = createDummyActionServer(
      mt, [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
      [&status](const types::DummyType& request, Publisher<types::DummyPrimitivesType>& status_publisher,
                std::atomic_bool&) {
        auto success = status_publisher.publish(status);
        EXPECT_TRUE(success);
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
      mt, [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
      [&requested_started](const types::DummyType& request,
                           [[maybe_unused]] Publisher<types::DummyPrimitivesType>& status_publisher,
                           std::atomic_bool& stop_requested) {
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
      [](const types::DummyPrimitivesType&) {}, SERVICE_CALL_TIMEOUT);

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
      mt, [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
      [&requested_started, &stop](const types::DummyType& request, Publisher<types::DummyPrimitivesType>&,
                                  std::atomic_bool&) {
        requested_started.test_and_set();
        requested_started.notify_all();

        stop.wait(false);

        return request;
      });

  auto request = types::DummyType::random(mt);
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request,
      [](const types::DummyPrimitivesType&) {}, SERVICE_CALL_TIMEOUT);

  auto other_reply = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
                         action_server_data.session, action_server_data.topic_config, request,
                         [](const types::DummyPrimitivesType&) {}, SERVICE_CALL_TIMEOUT)
                         .get();
  EXPECT_EQ(other_reply.status, RequestStatus::REJECTED_ALREADY_RUNNING);

  // Stop the original request
  stop.test_and_set();
  stop.notify_all();
  reply_future.get();
}

TEST(ActionServer, TypesMismatch) {
  auto mt = random::createRNG();

  auto action_server_data = createDummyActionServer(
      mt, [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
      [](const types::DummyType& request, Publisher<types::DummyPrimitivesType>& status_publisher,
         std::atomic_bool&) {
        auto success = status_publisher.publish({});
        EXPECT_TRUE(success);
        return request;
        return request;
      });

  // Invalid Request
  if (false) {
    auto request = types::DummyPrimitivesType::random(mt);
    const auto reply =
        callActionServer<types::DummyPrimitivesType, types::DummyPrimitivesType, types::DummyType>(
            action_server_data.session, action_server_data.topic_config, request,
            [](const types::DummyPrimitivesType&) {}, SERVICE_CALL_TIMEOUT)
            .get();

    EXPECT_EQ(reply.status, RequestStatus::INVALID);
  }

  // Invalid Status
  if (false) {
    auto request = types::DummyType::random(mt);
    const auto reply = callActionServer<types::DummyType, types::DummyType, types::DummyType>(
                           action_server_data.session, action_server_data.topic_config, request,
                           [](const types::DummyType&) {}, SERVICE_CALL_TIMEOUT)
                           .get();

    EXPECT_EQ(reply.status, RequestStatus::INVALID);
  }
}

}  // namespace
}  // namespace heph::ipc::zenoh::action_server::tests
