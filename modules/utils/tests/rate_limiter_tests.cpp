//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstddef>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/utils/timing/rate_limiter.h"

namespace heph::utils::timing::tests {

TEST(RateLimiter, LimitRate) {
  const auto rate = std::chrono::milliseconds(2);
  RateLimiter rate_limiter(rate);

  auto cb_count = size_t{ 0 };
  auto cb = [&cb_count]() -> size_t { return ++cb_count; };

  constexpr auto ITERATION_RATE = std::chrono::milliseconds(1);
  constexpr auto ITERATION_COUNT = 10;
  constexpr auto ITERATION_DURATION = ITERATION_RATE * ITERATION_COUNT;
  const auto end_time = std::chrono::steady_clock::now() + ITERATION_DURATION;
  while (std::chrono::steady_clock::now() < end_time) {
    rate_limiter(cb);
    constexpr auto SLEEP_TIME = std::chrono::milliseconds(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
  }

  // We expect cb_count == 5. To avoid flakiness, we allow for a margin of error in the count.
  EXPECT_GT(cb_count, 1);
  EXPECT_LT(cb_count, ITERATION_COUNT);
}

TEST(RateLimiter, LimitRateWithLongCallback) {
  const auto rate = std::chrono::milliseconds(1);
  RateLimiter rate_limiter(rate);

  auto cb_count = size_t{ 0 };
  auto long_cb = [&cb_count]() -> size_t {
    constexpr auto SLEEP_TIME = std::chrono::milliseconds(6);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    return ++cb_count;
  };

  constexpr auto ITERATION_DURATION = std::chrono::milliseconds(16);
  const auto end_time = std::chrono::steady_clock::now() + ITERATION_DURATION;
  while (std::chrono::steady_clock::now() < end_time) {
    rate_limiter(long_cb);
  }

  // We expect cb_count == 3 within 16 ms of testing, with a callback duration of 6 ms. The first callback
  // happens immediately, and the next two callbacks happen at 6 ms and 12 ms.
  EXPECT_LE(cb_count, 3);
}

}  // namespace heph::utils::timing::tests
