#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/service_client.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

namespace heph::ipc::zenoh::tests {
namespace {
// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

TEST(ZenohTests, ServiceCallExchange) {
  auto mt = random::createRNG();

  const auto request_message = types::DummyType::random(mt);

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  auto session = createSession(createLocalConfig());

  auto service_server = Service<types::DummyType, types::DummyType>(
      session, service_topic, [](const types::DummyType& request) { return request; });

  auto service_client = ServiceClient<types::DummyType, types::DummyType>(session, service_topic);
  const auto replies = service_client.call(request_message, std::chrono::milliseconds(10));
  EXPECT_FALSE(replies.empty());
  EXPECT_EQ(replies.size(), 1);
  EXPECT_EQ(replies.front().topic, service_topic.name);
  EXPECT_EQ(replies.front().value, request_message);
}

TEST(ZenohTests, TypesMismatch) {
  auto mt = random::createRNG();

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  auto session = createSession(createLocalConfig());

  std::size_t failed_requests = 0;
  auto service_server = Service<types::DummyType, types::DummyType>(
      session, service_topic, [](const types::DummyType& request) { return request; },
      [&failed_requests]() { ++failed_requests; });

  // Invalid request
  {
    const auto replies = callService<types::DummyPrimitivesType, types::DummyType>(
        *session, service_topic, types::DummyPrimitivesType::random(mt), std::chrono::milliseconds(10));
    EXPECT_THAT(replies, IsEmpty());
    EXPECT_EQ(failed_requests, 1);
  }

  // Invalid reply
  {
    const auto replies = callService<types::DummyType, types::DummyPrimitivesType>(
        *session, service_topic, types::DummyType::random(mt), std::chrono::milliseconds(10));
    EXPECT_THAT(replies, IsEmpty());
    EXPECT_EQ(failed_requests, 2);
  }
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
