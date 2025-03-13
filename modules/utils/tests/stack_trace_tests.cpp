//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================
#include <sstream>

#include <bits/types/stack_t.h>
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/stack_trace.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::stack_trace::tests {
TEST(StackTrace, print) {
  heph::utils::StackTrace stack_trace;

  std::string trace = heph::utils::StackTrace::print();
  fmt::println("{}", trace);
  ASSERT_FALSE(trace.empty());
}
}  // namespace heph::utils::stack_trace::tests
