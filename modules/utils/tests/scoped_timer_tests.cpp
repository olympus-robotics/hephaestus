//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

#include "hephaestus/test_utils/heph_test.h"
#include "hephaestus/utils/timing/mock_clock.h"
#include "hephaestus/utils/timing/scoped_timer.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::timing::tests {
namespace {

struct ScopedTimerTest : heph::test_utils::HephTest {};

TEST_F(ScopedTimerTest, ScopedTimer) {
  constexpr auto DURATION = std::chrono::milliseconds{ 42 };

  ScopedTimer::ClockT::duration elapsed;
  {
    ScopedTimer timer{ [&elapsed](ScopedTimer::ClockT::duration duration) { elapsed = duration; },
                       MockClock::now };
    MockClock::advance(DURATION);
  }
  EXPECT_EQ(elapsed, DURATION);
}
}  // namespace
}  // namespace heph::utils::timing::tests
