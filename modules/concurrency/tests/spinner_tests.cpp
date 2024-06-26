//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <chrono>
#include <ratio>

#include "gtest/gtest.h"
#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/exception.h"

class TestSpinner : public heph::concurrency::Spinner {
public:
  explicit TestSpinner(std::chrono::microseconds period = std::chrono::microseconds{ 0 }) : Spinner(period) {
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
  static constexpr auto PERIOD = std::chrono::microseconds{ 1000 };
  static constexpr auto PERIOD_MULTIPLIER = 10;

  TestSpinner spinner{ PERIOD };
  spinner.start();
  std::this_thread::sleep_for(PERIOD_MULTIPLIER * PERIOD);
  spinner.stop().get();

  EXPECT_GT(spinner.counter.load(), 8);
  EXPECT_LT(spinner.counter.load(), 10);
}
