#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

namespace heph::ipc::tests {

namespace {
void checkMessageExchange(bool subscriber_dedicated_callback_thread) {
  auto mt = random::createRNG();
  ipc::zenoh::Config config{};
  auto session = ipc::zenoh::createSession(std::move(config));
  const auto topic =
      ipc::TopicConfig(fmt::format("test_topic/{}", random::random<std::string>(mt, 10, false, true)));

  const auto send_message = types::DummyType::random(mt);
  auto received_message = types::DummyType::random(mt);
  EXPECT_NE(received_message, send_message);

  std::atomic_flag stop_flag = ATOMIC_FLAG_INIT;

  // Create publisher and subscriber
  ipc::zenoh::Publisher<types::DummyType> publisher(session, topic);

  auto subscriber = ipc::zenoh::createSubscriber<types::DummyType>(
      session, topic,
      [&received_message, &stop_flag]([[maybe_unused]] const ipc::zenoh::MessageMetadata& metadata,
                                      const std::shared_ptr<types::DummyType>& message) {
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

TEST(ZenohTests, MessageExchange) {
  checkMessageExchange(false);
  checkMessageExchange(true);
}

}  // namespace
}  // namespace heph::ipc::tests
