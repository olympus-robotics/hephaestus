//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <cstddef>
#include <memory>

#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/zenoh_publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // IWYU pragma: keep

namespace heph::conduit {
TEST(ZenohPublisher, BasicTest) {
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
  static constexpr auto VALUE = 42.;
  const auto topic_config = ipc::TopicConfig{ "test/output/topic" };
  auto zenoh_session = ipc::zenoh::createSession(ipc::zenoh::createLocalConfig());
  Output<types::DummyType> output{ "output" };
  const ZenohPublisher<types::DummyType> publisher(output, zenoh_session, topic_config);

  std::atomic_flag done = ATOMIC_FLAG_INIT;
  auto subscriber = ipc::zenoh::Subscriber<types::DummyType>(
      zenoh_session, topic_config, [&done](const auto&, const std::shared_ptr<types::DummyType>& msg) {
        EXPECT_EQ(msg->dummy_primitives_type.dummy_double, VALUE);
        done.test_and_set();
        done.notify_all();
      });

  types::DummyType msg;
  msg.dummy_primitives_type.dummy_double = VALUE;
  output(msg);

  heph::concurrency::Context context{ {} };
  stdexec::sync_wait(output.trigger(context.scheduler()));
  done.wait(false);
}
}  // namespace heph::conduit
