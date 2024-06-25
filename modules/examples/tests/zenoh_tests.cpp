#include <chrono>
#include <random>

#include <gtest/gtest.h>
#include <hephaestus/ipc/common.h>
#include <hephaestus/ipc/publisher.h>
#include <hephaestus/ipc/subscriber.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <hephaestus/ipc/zenoh/subscriber.h>

#include "helpers.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"

namespace heph::examples::types::tests {

namespace {
constexpr auto kSeed = 42;  // NOLINT(readability-identifier-naming)
}  // namespace

TEST(ZenohTests, MessageExchange) {
  std::mt19937_64 mt{ kSeed };  // NOLINT
  ipc::Config config{};
  auto session = ipc::zenoh::createSession(std::move(config));
  const auto topic = ipc::TopicConfig("test_topic");

  const auto send_message = randomPose(mt);
  auto received_message = randomPose(mt);
  EXPECT_NE(received_message, send_message);

  // Create publisher and subscriber
  ipc::Publisher<Pose> publisher(session, topic);
  auto subscriber = ipc::subscribe<heph::ipc::zenoh::Subscriber, Pose>(
      session, topic,
      [&received_message]([[maybe_unused]] const ipc::MessageMetadata& metadata,
                          const std::shared_ptr<Pose>& message) { received_message = *message; });

  const auto success = publisher.publish(send_message);
  EXPECT_TRUE(success);

  // Give enough time for the message to be received
  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_EQ(send_message, received_message);
}

TEST(IPCExampleClientTest, ServiceCallExchange) {
  std::mt19937_64 mt{ kSeed };  // NOLINT

  const auto request_message = randomPose(mt);
  auto response_message = randomPose(mt);
  EXPECT_NE(response_message, response_message);

  const auto service_topic = ipc::TopicConfig("test_service");
  ipc::Config config{};
  auto session = ipc::zenoh::createSession(std::move(config));

  auto service_server =
      ipc::zenoh::Service<Pose, Pose>(session, service_topic, [](const Pose& request) { return request; });

  const auto reply = ipc::zenoh::callService<Pose, Pose>(*session, service_topic, request_message,
                                                         std::chrono::milliseconds(1000));
  EXPECT_FALSE(reply.empty());
  EXPECT_EQ(reply.size(), 1);
  EXPECT_EQ(reply.front().topic, service_topic.name);
  EXPECT_EQ(reply.front().value, request_message);
}

}  // namespace heph::examples::types::tests
