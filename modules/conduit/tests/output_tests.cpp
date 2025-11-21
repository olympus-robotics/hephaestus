//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>

#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/types_proto/numeric_value.h"  // IWYU pragma: keep

namespace heph::conduit {

TEST(Output, NoPropagate) {
  Output<int> output{ "output" };

  output(0);

  stdexec::sync_wait(output.trigger({}));

  output(0);
}
TEST(Output, Propagate) {
  heph::concurrency::Context context{ {} };
  Input<int> input{ "input" };
  Output<int> output{ "output" };

  output.connect(input);
  EXPECT_FALSE(input.hasValue());

  output(0);

  stdexec::sync_wait(
      stdexec::when_all(output.trigger(context.scheduler()), input.trigger(context.scheduler())));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 0);

  output(0);
  EXPECT_FALSE(input.hasValue());
  stdexec::sync_wait(output.trigger(context.scheduler()));
  EXPECT_FALSE(input.hasValue());
  stdexec::sync_wait(input.trigger(context.scheduler()));
  EXPECT_TRUE(input.hasValue());
  EXPECT_EQ(input.value(), 0);
}
}  // namespace heph::conduit
