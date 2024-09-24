#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <random>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

#include "helpers.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/geometry.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/examples/types_protobuf/pose.h"      // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/random/random_number_generator.h"

namespace heph::examples::types::tests {

namespace {
constexpr auto kSeed = 42;  // NOLINT(readability-identifier-naming)
}  // namespace

void checkMessageExchange(bool subscriber_dedicated_callback_thread) {
  auto mt = random::createRNG();
  ipc::zenoh::Config config{};
  auto session = ipc::zenoh::createSession(std::move(config));
  const auto topic = ipc::TopicConfig("test_topic");

  const auto send_message = randomPose(mt);
  auto received_message = randomPose(mt);
  EXPECT_NE(received_message, send_message);

  std::atomic_flag stop_flag = ATOMIC_FLAG_INIT;

  // Create publisher and subscriber
  ipc::zenoh::Publisher<Pose> publisher(session, topic);

  auto subscriber = ipc::zenoh::createSubscriber<Pose>(
      session, topic,
      [&received_message, &stop_flag]([[maybe_unused]] const ipc::zenoh::MessageMetadata& metadata,
                                      const std::shared_ptr<Pose>& message) {
        received_message = *message;
        stop_flag.test_and_set();
        stop_flag.notify_all();
      },
      subscriber_dedicated_callback_thread);

  const auto success = publisher.publish(send_message);
  EXPECT_TRUE(success);

  // Wait to receive the message
  stop_flag.wait(false);

  EXPECT_EQ(send_message, received_message);
}

TEST(ZenohTests, WrongSubsriberTypeLargeIntoSmall) {
  auto mt = random::createRNG();
  ipc::zenoh::Config config{};
  auto session = ipc::zenoh::createSession(std::move(config));
  const auto topic = ipc::TopicConfig("test_topic");

  const auto send_message = randomFramedPose(mt);
  auto received_message = Pose{};

  std::atomic_flag stop_flag = ATOMIC_FLAG_INIT;

  ipc::zenoh::Publisher<FramedPose> publisher(session, topic);
  auto subscriber = ipc::zenoh::createSubscriber<Pose>(
      session, topic,
      [&received_message, &stop_flag]([[maybe_unused]] const ipc::zenoh::MessageMetadata& metadata,
                                      const std::shared_ptr<Pose>& message) {
        received_message = *message;
        stop_flag.test_and_set();
        stop_flag.notify_all();
      });

  EXPECT_THROW(std::ignore = publisher.publish(send_message);, std::exception);
}

TEST(ZenohTests, WrongSubsriberTypeSmallIntoLarge) {
  auto mt = random::createRNG();
  ipc::zenoh::Config config{};
  auto session = ipc::zenoh::createSession(std::move(config));
  const auto topic = ipc::TopicConfig("test_topic");

  const auto send_message = randomPose(mt);
  auto received_message = FramedPose{};

  std::atomic_flag stop_flag = ATOMIC_FLAG_INIT;

  ipc::zenoh::Publisher<Pose> publisher(session, topic);
  auto subscriber = ipc::zenoh::createSubscriber<FramedPose>(
      session, topic,
      [&received_message, &stop_flag]([[maybe_unused]] const ipc::zenoh::MessageMetadata& metadata,
                                      const std::shared_ptr<FramedPose>& message) {
        received_message = *message;
        stop_flag.test_and_set();
        stop_flag.notify_all();
      });

  EXPECT_THROW(std::ignore = publisher.publish(send_message);, std::exception);
}

TEST(ZenohTests, MessageExchange) {
  checkMessageExchange(false);
  checkMessageExchange(true);
}

TEST(ZenohTests, ServiceCallExchange) {
  std::mt19937_64 mt{ kSeed };  // NOLINT

  const auto request_message = randomPose(mt);
  auto response_message = randomPose(mt);
  EXPECT_NE(request_message, response_message);

  const auto service_topic = ipc::TopicConfig("test_service");

  ipc::zenoh::Config server_config{};
  auto server_session = ipc::zenoh::createSession(std::move(server_config));
  auto service_server = ipc::zenoh::Service<Pose, Pose>(server_session, service_topic,
                                                        [](const Pose& request) { return request; });

  ipc::zenoh::Config client_config{};
  auto client_session = ipc::zenoh::createSession(std::move(client_config));
  const auto reply = ipc::zenoh::callService<Pose, Pose>(*client_session, service_topic, request_message,
                                                         std::chrono::milliseconds(10));
  EXPECT_FALSE(reply.empty());
  EXPECT_EQ(reply.size(), 1);
  EXPECT_EQ(reply.front().topic, service_topic.name);
  EXPECT_EQ(reply.front().value, request_message);
}

}  // namespace heph::examples::types::tests
