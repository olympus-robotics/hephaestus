//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "gtest/gtest.h"
#include "hephaestus/concurrency/spinner.h"

class TestSpinner : public heph::concurrency::Spinner {
public:
  std::atomic<int> counter = 0;

protected:
  void spinOnce() override {
    ++counter;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // simulate work
  }
};

TEST(SpinnerTest, StartStopTest) {
  TestSpinner spinner;

  // Spinner should not be started initially.
  EXPECT_THROW(spinner.stop(), InvalidOperationException);

  // Start the spinner.
  spinner.start();

  // Spinner should not be able to start again.
  EXPECT_THROW(spinner.start(), InvalidOperationException);

  // Stop the spinner.
  spinner.stop();

  // Spinner should not be able to stop again.
  EXPECT_THROW(spinner.stop(), InvalidOperationException);
}

TEST(SpinnerTest, CallbackTest) {
  TestSpinner spinner;
  bool callback_called = false;

  // Set a callback that sets callback_called to true.
  spinner.addStopCallback([&]() { callback_called = true; });

  // Start and stop the spinner.
  spinner.start();
  spinner.stop();

  // The callback should have been called.
  EXPECT_TRUE(callback_called);
}

TEST(SpinnerTest, SpinTest) {
  TestSpinner spinner;

  // Start the spinner.
  spinner.start();

  // Wait for a while to let the spinner do some work.
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Stop the spinner.
  spinner.stop();

  // The counter should have been incremented.
  EXPECT_GT(spinner.counter.load(), 0);
}
