//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include <absl/log/globals.h>
#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/action_server/action_server_client.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/utils/stack_trace.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::action_server::tests {
namespace {

constexpr auto SERVICE_CALL_TIMEOUT = std::chrono::milliseconds{ 10 };
constexpr auto REPLY_SERVICE_TIMEOUT = std::chrono::seconds{ 5 };

class MyEnvironment : public Environment {
public:
  ~MyEnvironment() override = default;
  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
    absl::SetGlobalVLogLevel(3);
  }

private:
  heph::utils::StackTrace stack_trace_;
};

const auto* const my_env = AddGlobalTestEnvironment(new MyEnvironment{});  // NOLINT

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
  static constexpr int TOPIC_LENGTH = 20;
  auto service_topic = ipc::TopicConfig(
      fmt::format("test_action_server/{}", random::random<std::string>(mt, TOPIC_LENGTH, false, true)));

  auto server_session = createSession(createLocalConfig());

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

  auto action_server_client =
      ActionServerClient<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
          action_server_data.session, action_server_data.topic_config, [](const auto&) {},
          SERVICE_CALL_TIMEOUT);

  auto request = types::DummyType::random(mt);
  auto reply_future = action_server_client.call(request);

  const auto wait_res = reply_future.wait_for(REPLY_SERVICE_TIMEOUT);
  ASSERT_EQ(wait_res, std::future_status::ready);

  EXPECT_EQ(reply_future.get().status, RequestStatus::REJECTED_USER);
}

TEST(ActionServer, ActionServerClient) {
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

  types::DummyPrimitivesType received_status;
  std::atomic_flag received_status_flag = ATOMIC_FLAG_INIT;
  auto action_server_client =
      ActionServerClient<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
          action_server_data.session, action_server_data.topic_config,
          [&received_status, &received_status_flag](const types::DummyPrimitivesType& status) {
            received_status = status;
            received_status_flag.test_and_set();
            received_status_flag.notify_all();
          },
          SERVICE_CALL_TIMEOUT);

  // Test that the client can be reused multiple times.
  for (int i = 0; i < 3; ++i) {
    auto request = types::DummyType::random(mt);
    auto reply_future = action_server_client.call(request);

    received_status_flag.wait(false);
    EXPECT_EQ(status, received_status);
    received_status_flag.clear();

    const auto wait_res = reply_future.wait_for(REPLY_SERVICE_TIMEOUT);
    ASSERT_EQ(wait_res, std::future_status::ready);

    const auto reply = reply_future.get();
    EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
    EXPECT_EQ(reply.value, request);
  }
  heph::log(heph::DEBUG, "ActionServerClient test done");
}
/*
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
  std::atomic_flag received_status_flag = ATOMIC_FLAG_INIT;
  static constexpr auto REPLY_SERVICE_DEFAULT_TIMEOUT = std::chrono::milliseconds{ 10000 };
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request,
      [&received_status, &received_status_flag](const types::DummyPrimitivesType& status) {
        received_status = status;
        received_status_flag.test_and_set();
        received_status_flag.notify_all();
      },
      REPLY_SERVICE_DEFAULT_TIMEOUT);

  const auto reply = reply_future.get();
  EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
  EXPECT_EQ(reply.value, request);
}
*/

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

  auto action_server_client =
      ActionServerClient<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
          action_server_data.session, action_server_data.topic_config, [](const auto&) {},
          SERVICE_CALL_TIMEOUT);

  auto request = types::DummyType::random(mt);
  auto reply_future = action_server_client.call(request);

  // might spuriously wake up
  while (requested_started.test() == false) {
    requested_started.wait(false);
  }

  // requested_started guarantee that the request is now being proceed, but, although the action server create
  // the stop request service before processing the request, the stop service could still bootstrapping, as
  // this is controlled by Zenoh.
  // For this reason there is the chance that we need to try multiple times to stop the action server.
  while (true) {
    auto success =
        requestActionServerToStopExecution(*action_server_data.session, action_server_data.topic_config);
    if (success) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(4));
  };

  const auto reply = reply_future.get();

  EXPECT_EQ(reply.status, RequestStatus::STOPPED);
  EXPECT_EQ(reply.value, request);

  heph::log(heph::DEBUG, "ActionServerStopRequest test done");
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

        // might spuriously wake up
        while (stop.test() == false) {
          stop.wait(false);
        }

        return request;
      });

  auto action_server_client =
      ActionServerClient<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
          action_server_data.session, action_server_data.topic_config, [](const auto&) {},
          SERVICE_CALL_TIMEOUT);

  auto request = types::DummyType::random(mt);
  auto reply_future = action_server_client.call(request);

  // Calling from the same client will be rejected
  {
    auto other_reply_future = action_server_client.call(request);
    EXPECT_EQ(other_reply_future.get().status, RequestStatus::REJECTED_ALREADY_RUNNING);
  }
  // Calling from another client will be rejected
  {
    auto other_action_server_client =
        ActionServerClient<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
            action_server_data.session, action_server_data.topic_config, [](const auto&) {},
            SERVICE_CALL_TIMEOUT);
    auto other_reply_future = other_action_server_client.call(request);
    EXPECT_EQ(other_reply_future.get().status, RequestStatus::REJECTED_ALREADY_RUNNING);
  }

  // Stop the original request
  stop.test_and_set();
  stop.notify_all();
  reply_future.get();

  heph::log(heph::DEBUG, "ActionServerRejectedAlreadyRunning test done");
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
    auto reply_future =
        callActionServer<types::DummyPrimitivesType, types::DummyPrimitivesType, types::DummyType>(
            action_server_data.session, action_server_data.topic_config, request,
            [](const types::DummyPrimitivesType&) {}, SERVICE_CALL_TIMEOUT);
    const auto wait_res = reply_future.wait_for(REPLY_SERVICE_TIMEOUT);
    ASSERT_EQ(wait_res, std::future_status::ready);
    EXPECT_EQ(reply_future.get().status, RequestStatus::INVALID);
  }

  // Invalid Status
  if (false) {
    auto request = types::DummyType::random(mt);
    auto reply_future = callActionServer<types::DummyType, types::DummyType, types::DummyType>(
        action_server_data.session, action_server_data.topic_config, request, [](const types::DummyType&) {},
        SERVICE_CALL_TIMEOUT);

    const auto wait_res = reply_future.wait_for(REPLY_SERVICE_TIMEOUT);
    ASSERT_EQ(wait_res, std::future_status::ready);
    EXPECT_EQ(reply_future.get().status, RequestStatus::INVALID);
  }
}

}  // namespace
}  // namespace heph::ipc::zenoh::action_server::tests
