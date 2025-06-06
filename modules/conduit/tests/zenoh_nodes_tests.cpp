//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

#include <fmt/base.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/io_ring/timer.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/zenoh_nodes.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"
#include "hephaestus/utils/exception.h"

namespace heph::conduit::tests {

TEST(ZenohNodeTests, nodeBasic) {
  NodeEngine engine{ {} };

  auto zenoh_session = ipc::zenoh::createSession(ipc::zenoh::createLocalConfig());

  [[maybe_unused]] auto publisher_node = engine.createNode<ZenohPublisherNode<types::DummyType>>(
      zenoh_session, ipc::TopicConfig{ "test/output/topic" });

  auto subscriber_node =
      ZenohSubscriberNode(zenoh_session, ipc::TopicConfig{ "test/input/topic" }, publisher_node->input);

  auto publisher =
      ipc::zenoh::Publisher<types::DummyType>(zenoh_session, ipc::TopicConfig{ "test/input/topic" });

  // std::atomic_flag done = ATOMIC_FLAG_INIT;
  auto subscriber = ipc::zenoh::Subscriber<types::DummyType>(
      zenoh_session, ipc::TopicConfig{ "test/input/topic" },
      [&engine](const auto&, const std::shared_ptr<types::DummyType>& msg) {
        EXPECT_EQ(msg->dummy_primitives_type.dummy_double, 42.);
        engine.requestStop();
        // done.test_and_set();
        // done.notify_all();
      });

  types::DummyType msg;
  msg.dummy_primitives_type.dummy_double = 42.;
  const auto res = publisher.publish(msg);
  EXPECT_TRUE(res);
  engine.run();
}

}  // namespace heph::conduit::tests
