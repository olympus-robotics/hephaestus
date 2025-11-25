//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <memory>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

#include "helpers.h"
#include "hephaestus/error_handling/panic_exception.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_proto/geometry.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/examples/types_proto/pose.h"      // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/test_utils/heph_test.h"

namespace heph::examples::types::tests {
namespace {

struct ZenohTests : heph::test_utils::HephTest {};

TEST_F(ZenohTests, WrongSubscriberTypeLargeIntoSmall) {
  const ipc::zenoh::Config config{};
  auto session = ipc::zenoh::createSession(config);
  const auto topic = ipc::TopicConfig("test_topic");

  const auto send_message = randomFramedPose(mt());
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

  EXPECT_THROW(
      {
        std::ignore = publisher.publish(send_message);
        stop_flag.wait(false);
      },
      error_handling::PanicException);
}

TEST_F(ZenohTests, WrongSubscriberTypeSmallIntoLarge) {
  auto session = ipc::zenoh::createSession(ipc::zenoh::createLocalConfig());
  const auto topic = ipc::TopicConfig("test_topic");

  const auto send_message = randomPose(mt());
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

  EXPECT_THROW(
      {
        std::ignore = publisher.publish(send_message);
        stop_flag.wait(false);
      },
      error_handling::PanicException);
}

}  // namespace
}  // namespace heph::examples::types::tests
