//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <memory>
#include <string>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/serdes/serdes.h"
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
  ~MyEnvironment() override = default;
  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>(heph::DEBUG));
  }
};
// NOLINTNEXTLINE
const auto* const my_env = AddGlobalTestEnvironment(new MyEnvironment{});

TEST(ZenohTests, TopicDatabase) {
  auto mt = random::createRNG();

  auto session = createSession(createLocalConfig());

  const auto publisher_topic =
      TopicConfig(fmt::format("test_publisher/{}", random::random<std::string>(mt, 10, false, true)));
  auto publisher = Publisher<types::DummyType>{ session, publisher_topic };

  const auto subscriber_topic =
      TopicConfig(fmt::format("test_subscriber/{}", random::random<std::string>(mt, 10, false, true)));
  auto subscriber =
      Subscriber<types::DummyType>{ session, subscriber_topic, [](const auto&, const auto&) {} };

  const auto service_topic =
      TopicConfig(fmt::format("test_service/{}", random::random<std::string>(mt, 10, false, true)));
  auto service = Service<types::DummyType, types::DummyPrimitivesType>(
      session, service_topic, [](const auto&) { return types::DummyPrimitivesType{}; });

  const auto service_string_topic =
      TopicConfig(fmt::format("test_service_string/{}", random::random<std::string>(mt, 10, false, true)));
  auto service_string = Service<std::string, std::string>(session, service_string_topic,
                                                          [](const auto& request) { return request; });

  auto topic_database = createZenohTopicDatabase(session);

  auto publisher_type_info = topic_database->getTypeInfo(publisher_topic.name);
  EXPECT_EQ(publisher_type_info, serdes::getSerializedTypeInfo<types::DummyType>());

  auto subscriber_type_info = topic_database->getTypeInfo(subscriber_topic.name);
  EXPECT_EQ(subscriber_type_info, serdes::getSerializedTypeInfo<types::DummyType>());

  auto service_type_info = topic_database->getServiceTypeInfo(service_topic.name);
  EXPECT_TRUE(service_type_info.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(service_type_info->request, serdes::getSerializedTypeInfo<types::DummyType>());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(service_type_info->reply, serdes::getSerializedTypeInfo<types::DummyPrimitivesType>());

  auto service_string_type_info = topic_database->getServiceTypeInfo(service_string_topic.name);
  EXPECT_TRUE(service_string_type_info.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(service_string_type_info->request.name, "std::string");
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(service_string_type_info->reply.name, "std::string");

  auto result = topic_database->getTypeInfo("non_existent_topic");
  EXPECT_FALSE(result.has_value());
  auto service_result = topic_database->getServiceTypeInfo("non_existent_service");
  EXPECT_FALSE(service_result.has_value());
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
