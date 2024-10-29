#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

namespace heph::ipc::tests {

namespace {

TEST(ZenohTests, ServiceCallExchange) {
  auto mt = random::createRNG();

  const auto request_message = types::DummyType::random(mt);

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  ipc::zenoh::Config server_config{};
  auto server_session = ipc::zenoh::createSession(std::move(server_config));
  auto service_server = ipc::zenoh::Service<types::DummyType, types::DummyType>(
      server_session, service_topic, [](const types::DummyType& request) { return request; });

  ipc::zenoh::Config client_config{};
  auto client_session = ipc::zenoh::createSession(std::move(client_config));
  const auto reply = ipc::zenoh::callService<types::DummyType, types::DummyType>(
      *client_session, service_topic, request_message, std::chrono::milliseconds(10));
  EXPECT_FALSE(reply.empty());
  EXPECT_EQ(reply.size(), 1);
  EXPECT_EQ(reply.front().topic, service_topic.name);
  EXPECT_EQ(reply.front().value, request_message);
}

}  // namespace
}  // namespace heph::ipc::tests
