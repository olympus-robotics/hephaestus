#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/service_client.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

namespace heph::ipc::zenoh::tests {
namespace {
// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

class MyEnvironment : public Environment {
public:
  ~MyEnvironment() = default;
  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>(heph::DEBUG));
  }
};
// NOLINTNEXTLINE
const auto* const my_env = AddGlobalTestEnvironment(new MyEnvironment{});

TEST(ZenohTests, ServiceCallExchange) {
  auto mt = random::createRNG();

  const auto request_message = types::DummyType::random(mt);

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  auto session = createSession(createLocalConfig());

  auto service_server = Service<types::DummyType, types::DummyType>(
      session, service_topic, [](const types::DummyType& request) { return request; });

  const auto replies = callService<types::DummyType, types::DummyType>(
      *session, service_topic, request_message, std::chrono::milliseconds(10));

  EXPECT_FALSE(replies.empty());
  EXPECT_EQ(replies.size(), 1);
  EXPECT_EQ(replies.front().topic, service_topic.name);
  EXPECT_EQ(replies.front().value, request_message);
}

TEST(ZenohTests, ServiceClientCallExchange) {
  auto mt = random::createRNG();

  const auto request_message = types::DummyType::random(mt);

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  auto session = createSession(createLocalConfig());

  auto service_server = Service<types::DummyType, types::DummyType>(
      session, service_topic, [](const types::DummyType& request) { return request; });

  auto service_client =
      ServiceClient<types::DummyType, types::DummyType>(session, service_topic, std::chrono::seconds(1));
  const auto replies = service_client.call(request_message);
  EXPECT_FALSE(replies.empty());
  EXPECT_EQ(replies.size(), 1);
  EXPECT_EQ(replies.front().topic, service_topic.name);
  EXPECT_EQ(replies.front().value, request_message);
}

TEST(ZenohTests, ServiceCallRawExchange) {
  auto mt = random::createRNG();

  const auto request_message = types::DummyType::random(mt);

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  auto session = createSession(createLocalConfig());

  auto service_server = Service<types::DummyType, types::DummyType>(
      session, service_topic, [](const types::DummyType& request) { return request; });

  auto request_buffer = serdes::serialize(request_message);
  const auto replies = callServiceRaw(*session, service_topic, request_buffer, std::chrono::milliseconds(10));

  EXPECT_FALSE(replies.empty());
  EXPECT_EQ(replies.size(), 1);
  EXPECT_EQ(replies.front().topic, service_topic.name);
  auto reply_buffer = replies.front().value;

  types::DummyType reply;
  serdes::deserialize<types::DummyType>(reply_buffer, reply);
  EXPECT_EQ(reply, request_message);
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

TEST(ZenohTests, ServiceTypeInfo) {
  auto mt = random::createRNG();

  const auto request_message = types::DummyType::random(mt);

  const auto service_topic =
      ipc::TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));

  auto session = createSession(createLocalConfig());

  auto service_server = Service<types::DummyType, types::DummyPrimitivesType>(
      session, service_topic, [](const types::DummyType&) { return types::DummyPrimitivesType{}; });

  auto type_info_service_topic = TopicConfig{ getEndpointTypeInfoServiceTopic(service_topic.name) };
  EXPECT_TRUE(isEndpointTypeInfoServiceTopic(type_info_service_topic.name));

  auto service_client =
      ServiceClient<std::string, std::string>(session, type_info_service_topic, std::chrono::seconds(1));
  const auto replies = service_client.call("");
  EXPECT_FALSE(replies.empty());
  EXPECT_EQ(replies.size(), 1);
  EXPECT_EQ(replies.front().topic, type_info_service_topic.name);

  auto type_info = serdes::ServiceTypeInfo::fromJson(replies.front().value);
  EXPECT_EQ(type_info.request, serdes::getSerializedTypeInfo<types::DummyType>());
  EXPECT_EQ(type_info.reply, serdes::getSerializedTypeInfo<types::DummyPrimitivesType>());
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
