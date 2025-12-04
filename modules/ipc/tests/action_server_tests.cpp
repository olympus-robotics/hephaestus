//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/test_utils/heph_test.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::action_server::tests {
namespace {

constexpr auto SERVICE_CALL_TIMEOUT = std::chrono::milliseconds{ 10 };

class ActionServerTest : public heph::test_utils::HephTest {
public:
  ActionServerTest() : server_session_(createSession(createLocalConfig())) {
  }

  ~ActionServerTest() override {
    server_session_.reset();
  }

  ActionServerTest(const ActionServerTest&) = delete;
  ActionServerTest(ActionServerTest&&) = delete;

  auto operator=(const ActionServerTest&) -> ActionServerTest& = delete;
  auto operator=(ActionServerTest&&) -> ActionServerTest& = delete;

  [[nodiscard]] auto getSession() -> SessionPtr& {
    return server_session_;
  }

private:
  SessionPtr server_session_;
};

using DummyActionServer = ActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>;

struct ActionServerData {
  TopicConfig topic_config;
  SessionPtr session;
  std::unique_ptr<DummyActionServer> action_server;
};

[[nodiscard]] auto createDummyActionServer(std::mt19937_64& mt, SessionPtr& session,
                                           DummyActionServer::TriggerCallback&& trigger_cb,
                                           DummyActionServer::ExecuteCallback&& execute_cb)
    -> ActionServerData {
  static constexpr int TOPIC_LENGTH = 30;
  auto service_topic = ipc::TopicConfig(
      fmt::format("test_action_server/{}", random::random<std::string>(mt, TOPIC_LENGTH, false, true)));

  return {
    .topic_config = service_topic,
    .session = session,
    .action_server =
        std::make_unique<ActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>>(
            session, service_topic, std::move(trigger_cb), std::move(execute_cb)),
  };
}

TEST_F(ActionServerTest, RejectedCall) {
  auto action_server_data = createDummyActionServer(
      mt, getSession(), [](const types::DummyType&) { return TriggerStatus::REJECTED; },
      [&](const types::DummyType& request, Publisher<types::DummyPrimitivesType>&, std::atomic_bool&) {
        return request;
      });

  auto request = types::DummyType::random(mt);
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request, [](const auto&) {},
      SERVICE_CALL_TIMEOUT);

  EXPECT_EQ(reply_future.get().status, RequestStatus::REJECTED_USER);
}

TEST_F(ActionServerTest, ActionServerSuccessfulCall) {
  auto status = types::DummyPrimitivesType::random(mt);
  auto action_server_data = createDummyActionServer(
      mt, getSession(), [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
      [&status](const types::DummyType& request, Publisher<types::DummyPrimitivesType>& status_publisher,
                std::atomic_bool&) {
        auto success = status_publisher.publish(status);
        EXPECT_TRUE(success);
        return request;
      });

  auto request = types::DummyType::random(mt);
  std::mutex status_mtx;
  types::DummyPrimitivesType received_status;
  std::atomic_flag received_status_flag = ATOMIC_FLAG_INIT;
  static constexpr auto REPLY_SERVICE_DEFAULT_TIMEOUT = std::chrono::milliseconds{ 10000 };
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request,
      [&status_mtx, &received_status, &received_status_flag](const types::DummyPrimitivesType& dummy_status) {
        {
          const std::scoped_lock lock{ status_mtx };
          received_status = dummy_status;
        }
        received_status_flag.test_and_set();
        received_status_flag.notify_all();
      },
      REPLY_SERVICE_DEFAULT_TIMEOUT);

  while (received_status_flag.test() == false) {
    received_status_flag.wait(false);
  }
  {
    const std::scoped_lock lock{ status_mtx };
    EXPECT_EQ(status, received_status);
  }

  const auto reply = reply_future.get();
  EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
  EXPECT_EQ(reply.value, request);

  action_server_data.action_server.reset();

  action_server_data.session.reset();

  heph::log(heph::DEBUG, "ActionServerSuccessfulCall test done", "count",
            action_server_data.session.use_count());
}

TEST_F(ActionServerTest, ActionServerStopRequest) {
  std::atomic_flag requested_started = ATOMIC_FLAG_INIT;
  auto action_server_data = createDummyActionServer(
      mt, getSession(), [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
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
      action_server_data.session, action_server_data.topic_config, request, [](const auto&) {},
      SERVICE_CALL_TIMEOUT);

  // might spuriously wake up
  while (requested_started.test() == false) {
    requested_started.wait(false);
  }

  // requested_started guarantee that the request is now being proceed, but, although the action server
  // create the stop request service before processing the request, the stop service could still
  // bootstrapping, as this is controlled by Zenoh. For this reason there is the chance that we need to try
  // multiple times to stop the action server.
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

TEST_F(ActionServerTest, ActionServerRejectedAlreadyRunning) {
  std::atomic_flag requested_started = ATOMIC_FLAG_INIT;
  std::atomic_flag stop = ATOMIC_FLAG_INIT;
  auto action_server_data = createDummyActionServer(
      mt, getSession(), [](const types::DummyType&) { return TriggerStatus::SUCCESSFUL; },
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

  auto request = types::DummyType::random(mt);
  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      action_server_data.session, action_server_data.topic_config, request, [](const auto&) {},
      SERVICE_CALL_TIMEOUT);

  while (requested_started.test() == false) {
    requested_started.wait(false);
  }

  // Calling from another client will be rejected
  {
    auto other_reply_future =
        callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
            action_server_data.session, action_server_data.topic_config, request, [](const auto&) {},
            SERVICE_CALL_TIMEOUT * 2);
    EXPECT_EQ(other_reply_future.get().status, RequestStatus::REJECTED_ALREADY_RUNNING);
  }

  // Stop the original request
  stop.test_and_set();
  stop.notify_all();
  reply_future.get();
}

}  // namespace
}  // namespace heph::ipc::zenoh::action_server::tests
