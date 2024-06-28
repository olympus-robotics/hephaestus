//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <atomic>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/utils/timing/watchdog.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::timing::tests {
TEST(Watchdog, TimerFiring) {
  // TODO: a better way to write this test is to replace sleep_for with an atomic_flag, and then measure that
  // the elapsed time is more than N * period.
  constexpr auto PERIOD = std::chrono::milliseconds{ 10 };
  WatchdogTimer timer;
  std::atomic<int> count{ 0 };
  timer.start(PERIOD, [&count]() { ++count; });
  EXPECT_EQ(count, 0);
  std::this_thread::sleep_for(PERIOD * 4);
  timer.stop();

  EXPECT_GE(count, 3);
}

TEST(Watchdog, TimerFiringWithPath) {
  constexpr auto PERIOD = std::chrono::milliseconds{ 10 };
  constexpr auto PERIOD_EPSILON = PERIOD * 0.1;
  WatchdogTimer timer;
  std::atomic<int> count{ 0 };
  timer.start(PERIOD, [&count]() { ++count; });
  EXPECT_EQ(count, 0);
  timer.pat();
  std::this_thread::sleep_for(PERIOD + PERIOD_EPSILON);
  EXPECT_GE(count, 0);

  std::this_thread::sleep_for(PERIOD + PERIOD_EPSILON);
  timer.stop();

  EXPECT_GE(count, 1);
}

}  // namespace heph::utils::timing::tests
