//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "gtest/gtest.h"
#include "hephaestus/base/exception.h"
#include "hephaestus/concurrency/spinner.h"

class TestSpinner : public heph::concurrency::Spinner {
public:
  std::atomic<int> counter = 0;

protected:
  void spinOnce() override {
    ++counter;
    // simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // NOLINT
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
  TestSpinner spinner;

  spinner.start();

  // Wait for a while to let the spinner do some work.
  std::this_thread::sleep_for(std::chrono::seconds(1));
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
