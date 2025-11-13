//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <cstdlib>
#include <future>
#include <memory>

#include <gtest/gtest.h>

#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/zenoh_nodes.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

namespace heph::conduit::tests {

TEST(ZenohNodeTests, nodeBasic) {
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
  static constexpr auto VALUE = 42.;
  NodeEngine engine{ {} };

  auto zenoh_session = ipc::zenoh::createSession(ipc::zenoh::createLocalConfig());

  [[maybe_unused]] auto publisher_node =
      engine.createNode<ZenohPublisherNode<types::DummyType, "test_publisher">>(
          zenoh_session, ipc::TopicConfig{ "test/output/topic" });

  auto subscriber_node =
      ZenohSubscriberNode(zenoh_session, ipc::TopicConfig{ "test/input/topic" }, publisher_node->input);

  auto publisher =
      ipc::zenoh::Publisher<types::DummyType>(zenoh_session, ipc::TopicConfig{ "test/input/topic" });

  std::atomic_flag done = ATOMIC_FLAG_INIT;
  auto subscriber = ipc::zenoh::Subscriber<types::DummyType>(
      zenoh_session, ipc::TopicConfig{ "test/output/topic" },
      [&done](const auto&, const std::shared_ptr<types::DummyType>& msg) {
        EXPECT_EQ(msg->dummy_primitives_type.dummy_double, VALUE);
        done.test_and_set();
        done.notify_all();
      });

  auto future = std::async(std::launch::async, [&engine]() { engine.run(); });

  types::DummyType msg;
  msg.dummy_primitives_type.dummy_double = VALUE;
  const auto res = publisher.publish(msg);
  EXPECT_TRUE(res);

  done.wait(false);
  engine.requestStop();

  future.get();
}

}  // namespace heph::conduit::tests
