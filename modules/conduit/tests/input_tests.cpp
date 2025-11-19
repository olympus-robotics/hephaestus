//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstddef>

#include <exec/async_scope.hpp>
#include <exec/when_any.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/accumulated_input.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/forwarding_input.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/input_policy.h"
#include "hephaestus/types_proto/numeric_value.h"  // IWYU pragma: keep

namespace heph::conduit {

TEST(Input, DefaultPolicy) {
  heph::concurrency::Context context{ {} };

  Input<int> input{ "input" };

  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.setValue(1));
  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 1);
  auto last_trigger_time = input.lastTriggerTime();
  EXPECT_NE(last_trigger_time, ClockT::time_point{});

  stdexec::sync_wait(exec::when_any(stdexec::just(), input.trigger(context.scheduler())));
  EXPECT_FALSE(input.hasValue());
  EXPECT_EQ(last_trigger_time, input.lastTriggerTime());

  stdexec::sync_wait(input.setValue(3));
  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  stdexec::sync_wait(input.setValue(4));
  EXPECT_EQ(input.value(), 3);
  EXPECT_NE(last_trigger_time, input.lastTriggerTime());
}

TEST(Input, DefaultPolicyOverwrite) {
  heph::concurrency::Context context{ {} };

  Input<int, OVERWRITE_POLICY> input{ "input" };

  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.setValue(1));
  stdexec::sync_wait(input.setValue(2));
  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 2);

  stdexec::sync_wait(input.setValue(3));
  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  stdexec::sync_wait(input.setValue(4));
  EXPECT_EQ(input.value(), 3);
}

TEST(Input, ResetBlockingPolicy) {
  heph::concurrency::Context context{ {} };

  using InputPolicy = InputPolicy<KeepLastValuePolicy, BlockingTrigger>;
  Input<int> input{ "input", InputPolicy{} };

  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.setValue(1));
  EXPECT_FALSE(input.hasValue());

  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 1);
  auto last_trigger_time = input.lastTriggerTime();
  EXPECT_NE(last_trigger_time, ClockT::time_point{});

  stdexec::sync_wait(exec::when_any(stdexec::just(), input.trigger(context.scheduler())));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 1);
  EXPECT_EQ(last_trigger_time, input.lastTriggerTime());

  stdexec::sync_wait(input.setValue(3));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 1);

  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 3);
  EXPECT_NE(last_trigger_time, input.lastTriggerTime());
}

TEST(Input, BestEffortInputPolicy) {
  static constexpr std::chrono::milliseconds TIMEOUT{ 10 };

  Input<int> input{ "input", BestEffortInputPolicy{} };

  {
    heph::concurrency::Context context{ {} };

    input.setTimeout(TIMEOUT);

    EXPECT_FALSE(input.hasValue());

    stdexec::sync_wait(input.setValue(1));
    EXPECT_FALSE(input.hasValue());

    stdexec::sync_wait(input.trigger(context.scheduler()));
    EXPECT_TRUE(input.hasValue());
    EXPECT_EQ(input.value(), 1);
    auto last_trigger_time = input.lastTriggerTime();
    EXPECT_NE(last_trigger_time, ClockT::time_point{});

    stdexec::sync_wait(exec::when_any(stdexec::just(), input.trigger(context.scheduler())));
    EXPECT_TRUE(input.hasValue());
    EXPECT_EQ(input.value(), 1);
    EXPECT_EQ(last_trigger_time, input.lastTriggerTime());

    stdexec::sync_wait(input.setValue(3));
    EXPECT_TRUE(input.hasValue());
    EXPECT_EQ(input.value(), 1);

    stdexec::sync_wait(input.trigger(context.scheduler()));
    EXPECT_TRUE(input.hasValue());
    EXPECT_EQ(input.value(), 3);
    EXPECT_NE(last_trigger_time, input.lastTriggerTime());

    last_trigger_time = input.lastTriggerTime();

    exec::async_scope scope;
    scope.spawn(input.trigger(context.scheduler()) | stdexec::then([&]() { context.requestStop(); }));

    context.run();
    EXPECT_TRUE(input.hasValue());
    EXPECT_EQ(input.value(), 3);
    EXPECT_EQ(last_trigger_time, input.lastTriggerTime());
    EXPECT_NE(last_trigger_time, ClockT::now());
  }
  {
    heph::concurrency::Context context{ {} };
    exec::async_scope scope;
    auto last_trigger_time = input.lastTriggerTime();

    scope.spawn(input.trigger(context.scheduler()) | stdexec::then([&]() { context.requestStop(); }));
    scope.spawn(context.scheduler().schedule() | stdexec::let_value([&]() { return input.setValue(0); }));

    context.run();
    EXPECT_TRUE(input.hasValue());
    EXPECT_EQ(input.value(), 0);
    EXPECT_LT(last_trigger_time, input.lastTriggerTime());
    EXPECT_NE(last_trigger_time, ClockT::now());
  }
}

TEST(Input, AccumulatedInput) {
  heph::concurrency::Context context{ {} };

  AccumulatedInput<int, 3> input{ "input" };

  {
    exec::async_scope scope;
    scope.spawn(input.trigger(context.scheduler()));

    stdexec::sync_wait(input.setValue(0));

    scope.request_stop();
  }
  auto res = input.value();
  EXPECT_EQ(res.size(), 1);
  EXPECT_EQ(res[0], 0);
  auto last_trigger_time = input.lastTriggerTime();
  EXPECT_NE(last_trigger_time, ClockT::time_point{});

  {
    exec::async_scope scope;
    scope.spawn(input.trigger(context.scheduler()));
    scope.request_stop();
  }
  EXPECT_EQ(last_trigger_time, input.lastTriggerTime());
  res = input.value();
  EXPECT_EQ(res.size(), 0);

  {
    exec::async_scope scope;
    scope.spawn(input.trigger(context.scheduler()));

    stdexec::sync_wait(input.setValue(3));

    scope.request_stop();
  }
  res = input.value();
  EXPECT_EQ(res.size(), 1);
  EXPECT_EQ(res[0], 3);

  EXPECT_NE(last_trigger_time, input.lastTriggerTime());

  {
    exec::async_scope scope;

    scope.spawn(input.trigger(context.scheduler()));

    stdexec::sync_wait(input.setValue(0));
    stdexec::sync_wait(input.setValue(1));
    stdexec::sync_wait(input.setValue(2));
    stdexec::sync_wait(input.setValue(3));

    scope.request_stop();
    stdexec::sync_wait(scope.on_empty());
  }
  res = input.value();
  EXPECT_EQ(res.size(), 3);
  EXPECT_EQ(res[0], 1);
  EXPECT_EQ(res[1], 2);
  EXPECT_EQ(res[2], 3);
}

TEST(Input, ForwardingInput) {
  heph::concurrency::Context context{ {} };
  ForwardingInput<int> input{ "input0" };
  Input<int> input1{ "input1" };
  Input<int> input2{ "input2" };

  EXPECT_FALSE(input1.hasValue());
  EXPECT_FALSE(input2.hasValue());

  input.forward(input1);
  input.forward(input2);
  stdexec::sync_wait(stdexec::when_all(input.setValue(0), input1.trigger(context.scheduler()),
                                       input2.trigger(context.scheduler())));
  EXPECT_TRUE(input1.hasValue());
  EXPECT_TRUE(input2.hasValue());

  auto res = input1.value();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(res, input2.value());
}

}  // namespace heph::conduit
