#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/test_utils/heph_test.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::ipc::zenoh::tests {

namespace {
void checkMessageExchange(std::mt19937_64& mt, bool subscriber_dedicated_callback_thread) {
  auto session = createSession(createLocalConfig());
  const auto topic =
      ipc::TopicConfig(fmt::format("test_topic/{}", random::random<std::string>(mt, 10, false, true)));

  Publisher<types::DummyType> publisher(session, topic);

  types::DummyType received_message;
  std::atomic_flag stop_flag = ATOMIC_FLAG_INIT;
  SubscriberConfig config;
  config.dedicated_callback_thread = subscriber_dedicated_callback_thread;
  auto subscriber = createSubscriber<types::DummyType>(
      session, topic,
      [&received_message, &stop_flag]([[maybe_unused]] const MessageMetadata& metadata,
                                      const std::shared_ptr<types::DummyType>& message) {
        received_message = *message;
        stop_flag.test_and_set();
        stop_flag.notify_all();
      },
      config);

  const auto send_message = types::DummyType::random(mt);
  const auto success = publisher.publish(send_message);
  EXPECT_TRUE(success);

  // Wait to receive the message
  stop_flag.wait(false);

  EXPECT_EQ(send_message, received_message);
}

struct PublisherSubscriber : heph::test_utils::HephTest {};

TEST_F(PublisherSubscriber, MessageExchange) {
  checkMessageExchange(this->mt, false);
  checkMessageExchange(this->mt, true);
}

TEST_F(PublisherSubscriber, MismatchType) {
  const Config config{};
  auto session = createSession(createLocalConfig());
  const auto topic =
      ipc::TopicConfig(fmt::format("test_topic/{}", random::random<std::string>(mt, 10, false, true)));

  Publisher<types::DummyType> publisher(session, topic);

  std::atomic_flag stop_flag = ATOMIC_FLAG_INIT;
  auto subscriber = createSubscriber<types::DummyPrimitivesType>(
      session, topic,
      [&stop_flag]([[maybe_unused]] const MessageMetadata& metadata,
                   const std::shared_ptr<types::DummyPrimitivesType>& message) {
        (void)metadata;
        (void)message;
        stop_flag.test_and_set();
        stop_flag.notify_all();
      },
      {});

  EXPECT_THROW(
      {
        std::ignore = publisher.publish(types::DummyType::random(mt));
        stop_flag.wait(false);
      },
      std::runtime_error);
}

TEST_F(PublisherSubscriber, PublisherTypeInfo) {
  auto session = createSession(createLocalConfig());
  const auto topic =
      ipc::TopicConfig(fmt::format("test_topic/{}", random::random<std::string>(mt, 10, false, true)));

  const Publisher<types::DummyType> publisher(session, topic);

  auto type_info_json = callService<std::string, std::string>(
      *session, TopicConfig{ getEndpointTypeInfoServiceTopic(topic.name) }, "", std::chrono::seconds{ 1 });

  EXPECT_THAT(type_info_json, SizeIs(1));
  auto type_info = serdes::TypeInfo::fromJson(type_info_json.front().value);

  EXPECT_EQ(type_info, serdes::getSerializedTypeInfo<types::DummyType>());
}

TEST_F(PublisherSubscriber, SubscriberTypeInfo) {
  auto session = createSession(createLocalConfig());
  const auto topic =
      ipc::TopicConfig(fmt::format("test_topic/{}", random::random<std::string>(mt, 10, false, true)));

  const Subscriber<types::DummyType> subscriber(session, topic, [](const auto&, const auto&) {});

  auto type_info_json = callService<std::string, std::string>(
      *session, TopicConfig{ getEndpointTypeInfoServiceTopic(topic.name) }, "", std::chrono::seconds{ 1 });

  EXPECT_THAT(type_info_json, SizeIs(1));
  auto type_info = serdes::TypeInfo::fromJson(type_info_json.front().value);

  EXPECT_EQ(type_info, serdes::getSerializedTypeInfo<types::DummyType>());
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
