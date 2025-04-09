//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

#include "hephaestus/ipc/zenoh/action_server/pollable_action_server.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"     // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/numeric_value.h"  // NOLINT(misc-include-cleaner)

using namespace ::testing;             // NOLINT(google-build-using-namespace)
using namespace std::chrono_literals;  // NOLINT(google-build-using-namespace)

namespace heph::ipc::zenoh::action_server {

TEST(PollableActionServerTest, CompleteAction) {
  auto mt = random::createRNG();

  const auto session = createSession(createLocalConfig());
  const ipc::TopicConfig topic_config("test/polling_action_server");

  PollableActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType> action_server(
      session, topic_config);

  // Send 2 requests, to test if everything still works the second time.
  for (size_t i = 0; i < 2; i++) {
    const auto expected_request = types::DummyType::random(mt);
    const auto expected_reply = types::DummyType::random(mt);

    EXPECT_FALSE(action_server.pollRequest().has_value());

    auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
        session, topic_config, expected_request, [](const auto&) {}, 10ms);

    while (true) {
      const auto request_opt = action_server.pollRequest();
      if (request_opt.has_value()) {
        EXPECT_EQ(*request_opt, expected_request);
        break;
      }

      std::this_thread::yield();
    }

    EXPECT_EQ(reply_future.wait_for(0s), std::future_status::timeout);

    action_server.complete(expected_reply);

    const auto reply = reply_future.get();
    EXPECT_EQ(reply.status, RequestStatus::SUCCESSFUL);
    EXPECT_EQ(reply.value, expected_reply);
  }
}

TEST(PollableActionServerTest, StopExecution) {
  auto mt = random::createRNG();

  const auto session = createSession(createLocalConfig());
  const ipc::TopicConfig topic_config("test/polling_action_server");

  PollableActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType> action_server(
      session, topic_config);

  // Send 2 requests, to test if everything still works the second time.
  for (size_t i = 0; i < 2; i++) {
    const auto expected_request = types::DummyType::random(mt);
    const auto expected_reply = types::DummyType::random(mt);

    EXPECT_FALSE(action_server.pollRequest().has_value());

    auto reply_future = callActionServer<types::DummyType, types::DummyPrimitivesType, types::DummyType>(
        session, topic_config, expected_request, [](const auto&) {}, 10ms);

    while (true) {
      const auto request_opt = action_server.pollRequest();
      if (request_opt.has_value()) {
        EXPECT_EQ(*request_opt, expected_request);
        break;
      }

      std::this_thread::yield();
    }

    EXPECT_EQ(reply_future.wait_for(0s), std::future_status::timeout);

    EXPECT_TRUE(requestActionServerToStopExecution(*session, topic_config));

    while (action_server.shouldAbort()) {
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
  const ipc::TopicConfig topic_config("test/polling_action_server");

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
        10ms);

    while (true) {
      const auto request_opt = action_server.pollRequest();
      if (request_opt.has_value()) {
        EXPECT_EQ(*request_opt, expected_request);
        break;
      }

      std::this_thread::yield();
    }

    for (int j = 0; j < 10; j++) {
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

}  // namespace heph::ipc::zenoh::action_server
