//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <memory>

#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/zenoh_subscriber.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // IWYU pragma: keep

namespace heph::conduit {
TEST(ZenohSubscriber, BasicTest) {
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
  static constexpr auto VALUE = 42.;
  const auto topic_config = ipc::TopicConfig{ "test/input/topic" };
  auto zenoh_session = ipc::zenoh::createSession(ipc::zenoh::createLocalConfig());
  Input<types::DummyType> input{ "input" };
  const ZenohSubscriber<types::DummyType> subscriber(input, zenoh_session, topic_config);

  auto publisher = ipc::zenoh::Publisher<types::DummyType>(zenoh_session, topic_config);
  types::DummyType msg;
  msg.dummy_primitives_type.dummy_double = VALUE;
  auto res = publisher.publish(msg);
  EXPECT_TRUE(res);
  stdexec::sync_wait(input.trigger({}));
  EXPECT_TRUE(input.hasValue());
  auto value = input.value();
  EXPECT_EQ(value.dummy_primitives_type.dummy_double, VALUE);
}
}  // namespace heph::conduit
