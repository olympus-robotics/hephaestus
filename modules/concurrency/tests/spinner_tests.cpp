//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <atomic>
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::tests {

class TestSpinner : public heph::concurrency::Spinner {
public:
  explicit TestSpinner(double rate_hz = 0) : Spinner(rate_hz) {
  }

  std::atomic<int> counter = 0;

protected:
  void spinOnce() override {
    ++counter;
  }
};

TEST(SpinnerTest, StartStopTest) {
  TestSpinner spinner;

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
  spinner.start();

  EXPECT_THROW(spinner.start(), heph::InvalidOperationException);
  auto future = spinner.stop();
  future.get();

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
}

TEST(SpinnerTest, SpinTest) {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 1 };
  TestSpinner spinner;

  spinner.start();

  // Wait for a while to let the spinner do some work.
  std::this_thread::sleep_for(WAIT_FOR);
  auto future = spinner.stop();
  future.get();

  // The counter should have been incremented.
  EXPECT_GT(spinner.counter.load(), 0);
}

TEST(SpinnerTest, StopCallbackTest) {
  TestSpinner spinner;
  std::atomic<bool> callback_called(false);

  spinner.addStopCallback([&]() { callback_called.store(true); });

  spinner.start();
  auto future = spinner.stop();
  future.get();

  // The callback should have been called.
  EXPECT_TRUE(callback_called.load());
}

TEST(SpinnerTest, SpinWithPeriod) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  TestSpinner spinner{ RATE_HZ };
  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  spinner.stop().get();

  EXPECT_GT(spinner.counter.load(), 8);
  EXPECT_LT(spinner.counter.load(), 12);
}

}  // namespace heph::concurrency::tests
