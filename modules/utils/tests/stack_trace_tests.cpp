//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <fmt/base.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/stack_trace.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::stack_trace::tests {
TEST(StackTrace, print) {
  const StackTrace stack_trace;

  const auto trace = StackTrace::print();
  fmt::println("{}", trace);
  ASSERT_FALSE(trace.empty());
}
}  // namespace heph::utils::stack_trace::tests
