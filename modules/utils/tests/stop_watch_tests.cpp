//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

#include "hephaestus/utils/timing/mock_clock.h"
#include "hephaestus/utils/timing/stop_watch.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::timing::tests {
TEST(StopWatch, AccumulateTime) {
  constexpr auto PERIOD = std::chrono::milliseconds{ 100 };

  StopWatch swatch{ MockClock::now };
  swatch.start();
  const auto t1 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(t1.count(), 0);
  MockClock::advance(PERIOD);
  const auto t2 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(t2, t1 + PERIOD);
}

TEST(StopWatch, Stoppable) {
  static constexpr auto PERIOD = std::chrono::milliseconds{ 100 };

  StopWatch swatch{ MockClock::now };
  swatch.start();

  MockClock::advance(PERIOD);
  const auto elapsed = swatch.stop();
  EXPECT_EQ(swatch.lapsCount(), 1);
  EXPECT_EQ(elapsed, PERIOD);

  const auto t1 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(elapsed, t1);

  MockClock::advance(PERIOD);
  const auto t2 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(t1, t2);
}

TEST(StopWatch, ResumeCounting) {
  constexpr auto PERIOD = std::chrono::milliseconds{ 10 };

  StopWatch swatch{ MockClock::now };

  // start and let it run for period
  swatch.start();
  MockClock::advance(PERIOD);

  // stop for a while
  std::ignore = swatch.stop();
  MockClock::advance(PERIOD);

  // start again and let it run for period
  swatch.start();
  MockClock::advance(PERIOD);

  // should have accumulated about 2 periods of count
  const auto t2 = swatch.accumulatedLapsDuration();
  EXPECT_EQ(t2, 2 * PERIOD);

  std::ignore = swatch.stop();
  EXPECT_EQ(swatch.lapsCount(), 2);
}

TEST(StopWatch, Reset) {
  constexpr auto PERIOD = std::chrono::milliseconds{ 10 };

  StopWatch swatch{ MockClock::now };
  swatch.start();
  MockClock::advance(PERIOD);
  std::ignore = swatch.stop();

  EXPECT_EQ(swatch.accumulatedLapsDuration(), PERIOD);

  swatch.reset();
  EXPECT_EQ(swatch.accumulatedLapsDuration().count(), 0);
  EXPECT_EQ(swatch.lapsCount(), 0);
}

TEST(StopWatch, Lapse) {
  constexpr auto PERIOD = std::chrono::milliseconds{ 10 };

  StopWatch swatch{ MockClock::now };
  swatch.start();
  MockClock::advance(PERIOD);
  const auto l1 = swatch.lapse();
  EXPECT_EQ(l1, PERIOD);
  MockClock::advance(2 * PERIOD);
  const auto l2 = swatch.lapse();
  EXPECT_EQ(l2, 2 * PERIOD);
  const auto e = swatch.elapsed();
  EXPECT_EQ(e, l1 + l2);
  const auto d = swatch.stop();
  EXPECT_EQ(d, e);
}

TEST(StopWatch, DurationCast) {
  using namespace std::chrono_literals;
  using DurationT = std::chrono::duration<double>;
  StopWatch swatch{ MockClock::now };
  swatch.start();
  const auto elapsed = swatch.lapse<DurationT>();  // NOLINT(clang-analyzer-deadcode.DeadStores)
  static_assert(std::is_same_v<std::remove_const_t<decltype(elapsed)>, DurationT>);

  const auto total_elapsed = swatch.stop<DurationT>();  // NOLINT(clang-analyzer-deadcode.DeadStores)
  static_assert(std::is_same_v<std::remove_const_t<decltype(total_elapsed)>, DurationT>);
}

}  // namespace heph::utils::timing::tests
