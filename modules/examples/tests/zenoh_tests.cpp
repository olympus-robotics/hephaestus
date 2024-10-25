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
#include "hephaestus/examples/types_proto/geometry.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/examples/types_proto/pose.h"      // NOLINT(misc-include-cleaner)
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

}  // namespace heph::examples::types::tests
