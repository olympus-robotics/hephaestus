//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <string>
#include <thread>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/action_server/pollable_action_server.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"     // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/numeric_value.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/utils/exception.h"

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::ipc::zenoh::action_server {

static constexpr auto REQUEST_TIMEOUT = std::chrono::seconds{ 1 };
static constexpr auto ZERO_DURATION = std::chrono::milliseconds{ 0 };
static constexpr auto TOPIC_ID_LENGTH = 30;

TEST(PollableActionServerTest, CompleteAction) {
  auto mt = random::createRNG();

  const auto session = createSession(createLocalConfig());
  const ipc::TopicConfig topic_config(
      fmt::format("test/polling_action_server_{}/complete_action_test",
                  random::random<std::string>(mt, TOPIC_ID_LENGTH, false, true)));

  PollableActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType> action_server(
      session, topic_config);

  // Send 2 requests, to test if everything still works the second time.
  for (size_t i = 0; i < 2; i++) {
    const auto expected_request = types::DummyType::random(mt);
    const auto expected_reply = types::DummyType::random(mt);

    EXPECT_FALSE(action_server.pollRequest().has_value());

    auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
        session, topic_config, expected_request, [](const auto&) {}, REQUEST_TIMEOUT);
    heph::panicIf(!reply_future.valid(), "callActionServer failed.");

    while (true) {
      if (reply_future.wait_for(ZERO_DURATION) != std::future_status::timeout) {
        FAIL() << "reply_future completed too early";
      }

      const auto request_opt = action_server.pollRequest();
      if (request_opt.has_value()) {
        EXPECT_EQ(*request_opt, expected_request);
        break;
      }

      std::this_thread::yield();
    }

    if (reply_future.wait_for(ZERO_DURATION) != std::future_status::timeout) {
      FAIL() << "reply_future completed too early with result";
    }

    action_server.complete(expected_reply);

    const auto reply = reply_future.get();
    EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
    EXPECT_EQ(reply.value, expected_reply);
  }
}

TEST(PollableActionServerTest, StopExecution) {
  auto mt = random::createRNG();

  const auto session = createSession(createLocalConfig());
  const ipc::TopicConfig topic_config(
      fmt::format("test/polling_action_server_{}/stop_execution_action_test",
                  random::random<std::string>(mt, TOPIC_ID_LENGTH, false, true)));

  PollableActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType> action_server(
      session, topic_config);

  // Send 2 requests, to test if everything still works the second time.
  for (size_t i = 0; i < 2; i++) {
    const auto expected_request = types::DummyType::random(mt);
    const auto expected_reply = types::DummyType::random(mt);

    EXPECT_FALSE(action_server.pollRequest().has_value());

    auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
        session, topic_config, expected_request, [](const auto&) {}, REQUEST_TIMEOUT);
    heph::panicIf(!reply_future.valid(), "callActionServer failed.");

    while (true) {
      if (reply_future.wait_for(ZERO_DURATION) != std::future_status::timeout) {
        FAIL() << "reply_future completed too early";
      }

      const auto request_opt = action_server.pollRequest();
      if (request_opt.has_value()) {
        EXPECT_EQ(*request_opt, expected_request);
        break;
      }

      std::this_thread::yield();
    }

    if (reply_future.wait_for(ZERO_DURATION) != std::future_status::timeout) {
      FAIL() << "reply_future completed too early";
    }

    EXPECT_TRUE(requestActionServerToStopExecution(*session, topic_config));

    while (!action_server.shouldAbort()) {
      std::this_thread::yield();
    }

    action_server.complete(expected_reply);

    const auto reply = reply_future.get();
    EXPECT_EQ(reply.status, RequestStatus::STOPPED);
    EXPECT_EQ(reply.value, expected_reply);
  }
}

TEST(PollableActionServerTest, CompleteActionWithStatusUpdates) {
  auto mt = random::createRNG();

  const auto session = createSession(createLocalConfig());
  const ipc::TopicConfig topic_config(
      fmt::format("test/polling_action_server_{}/complete_action_with_status_updates_test",
                  random::random<std::string>(mt, TOPIC_ID_LENGTH, false, true)));

  PollableActionServer<types::DummyType, int, types::DummyType> action_server(session, topic_config);

  // Send 2 requests, to test if everything still works the second time.
  for (size_t i = 0; i < 2; i++) {
    const auto expected_request = types::DummyType::random(mt);
    const auto expected_reply = types::DummyType::random(mt);

    EXPECT_FALSE(action_server.pollRequest().has_value());

    std::atomic<int> expected_status{ -1 };
    std::atomic<int> last_received_status{ -1 };
    auto reply_future = callActionServer<types::DummyType, int, types::DummyType>(
        session, topic_config, expected_request,
        [&](const int& status) {
          EXPECT_EQ(status, expected_status.load());
          last_received_status.store(status);
        },
        REQUEST_TIMEOUT);
    heph::panicIf(!reply_future.valid(), "callActionServer failed.");

    while (true) {
      if (reply_future.wait_for(ZERO_DURATION) != std::future_status::timeout) {
        FAIL() << "reply_future completed too early";
      }

      const auto request_opt = action_server.pollRequest();
      if (request_opt.has_value()) {
        EXPECT_EQ(*request_opt, expected_request);
        break;
      }

      std::this_thread::yield();
    }

    static constexpr auto ITERATION_COUNT = 10;
    for (int j = 0; j < ITERATION_COUNT; ++j) {
      expected_status.store(j);
      action_server.setStatus(j);

      while (last_received_status.load() != expected_status) {
        std::this_thread::yield();
      }
    }

    action_server.complete(expected_reply);

    const auto reply = reply_future.get();
    EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
    EXPECT_EQ(reply.value, expected_reply);
  }
}

TEST(PollableActionServerTest, StopActionServer) {
  auto mt = random::createRNG();

  const auto session = createSession(createLocalConfig());
  const ipc::TopicConfig topic_config(
      fmt::format("test/polling_action_server_{}/stop_action_server_test",
                  random::random<std::string>(mt, TOPIC_ID_LENGTH, false, true)));

  PollableActionServer<types::DummyType, int, types::DummyType> action_server(session, topic_config);

  auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
      session, topic_config, types::DummyType::random(mt), [](const auto&) {}, REQUEST_TIMEOUT);
  heph::panicIf(!reply_future.valid(), "callActionServer failed.");

  while (!action_server.pollRequest().has_value()) {
    if (reply_future.wait_for(std::chrono::milliseconds{ 0 }) != std::future_status::timeout) {
      FAIL() << "reply_future completed too early";
    }

    std::this_thread::yield();
  }

  std::atomic_bool action_completed{ false };

  std::thread thread([&]() {
    action_server.stop();
    EXPECT_TRUE(action_completed.load());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
  action_completed.store(true);
  action_server.complete(types::DummyType::random(mt));
  reply_future.get();

  thread.join();
}

}  // namespace heph::ipc::zenoh::action_server
