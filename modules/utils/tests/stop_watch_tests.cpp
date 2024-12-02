//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/utils/timing/stop_watch.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::timing::tests {
TEST(StopWatch, AccumulateTime) {
  using namespace std::chrono_literals;
  constexpr auto PERIOD = 100ms;

  StopWatch swatch;
  swatch.start();
  const auto t1 = swatch.accumulatedLapsDuration();
  EXPECT_GT(t1.count(), 0);
  std::this_thread::sleep_for(PERIOD);
  const auto t2 = swatch.accumulatedLapsDuration();
  EXPECT_LT(t1, t2);
}

TEST(StopWatch, Stoppable) {
  using namespace std::chrono_literals;
  static constexpr auto PERIOD = 10ms;

  StopWatch swatch;
  swatch.start();

  std::this_thread::sleep_for(PERIOD);
  const auto elapsed = swatch.stop();
  EXPECT_EQ(swatch.lapsCount(), 1);

  const auto t1 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(elapsed, t1);
  EXPECT_GT(t1.count(), 0);

  std::this_thread::sleep_for(PERIOD);
  const auto t2 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(t1.count(), t2.count());
}

TEST(StopWatch, ResumeCounting) {
  using namespace std::chrono_literals;
  constexpr auto PERIOD = 10ms;

  StopWatch swatch;

  // start and let it run for period
  swatch.start();
  std::this_thread::sleep_for(PERIOD);

  // stop for a while
  std::ignore = swatch.stop();
  std::this_thread::sleep_for(PERIOD);

  // start again and let it run for period
  swatch.start();
  std::this_thread::sleep_for(PERIOD);
  // should have accumulated about 2 periods of count
  const auto t2 = swatch.accumulatedLapsDuration();
  EXPECT_GT(t2, 2 * PERIOD - 1ms);

  std::ignore = swatch.stop();
  EXPECT_EQ(swatch.lapsCount(), 2);
}

TEST(StopWatch, Reset) {
  using namespace std::chrono_literals;
  constexpr auto PERIOD = 10ms;

  StopWatch swatch;
  swatch.start();
  std::this_thread::sleep_for(PERIOD);
  std::ignore = swatch.stop();

  EXPECT_NE(swatch.accumulatedLapsDuration().count(), 0);

  swatch.reset();
  EXPECT_EQ(swatch.accumulatedLapsDuration().count(), 0);
  EXPECT_EQ(swatch.lapsCount(), 0);
}

TEST(StopWatch, Lapse) {
  using namespace std::chrono_literals;
  constexpr auto PERIOD = 10ms;

  StopWatch swatch;
  swatch.start();
  std::this_thread::sleep_for(PERIOD);
  const auto l1 = swatch.lapse();
  const auto l2 = swatch.lapse();
  const auto d = swatch.stop();
  EXPECT_GT(l1, l2);
  EXPECT_GT(d, l2);
}

TEST(StopWatch, DurationCast) {
  using namespace std::chrono_literals;
  using DurationT = std::chrono::duration<double>;
  StopWatch swatch;
  swatch.start();
  auto elapsed = swatch.lapse<DurationT>();
  static_assert(std::is_same_v<decltype(elapsed), DurationT>);

  auto total_elapsed = swatch.stop<DurationT>();
  static_assert(std::is_same_v<decltype(total_elapsed), DurationT>);
}

}  // namespace heph::utils::timing::tests