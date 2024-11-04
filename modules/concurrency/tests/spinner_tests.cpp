//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstddef>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::tests {

struct TestFixture : public ::testing::Test {
  static auto trivialCallback() -> Spinner::Callback {
    return []() {};
  }

  static auto stoppingCallback(size_t& callback_called_counter) -> Spinner::StoppableCallback {
    return [&callback_called_counter]() {
      static constexpr auto STOP_AFTER = 10;
      if (callback_called_counter < STOP_AFTER) {
        ++callback_called_counter;
        return Spinner::SpinResult::CONTINUE;
      }

      return Spinner::SpinResult::STOP;
    };
  }

  static auto nonThrowingCallback(size_t& callback_called_counter) -> Spinner::Callback {
    return [&callback_called_counter]() { ++callback_called_counter; };
  }

  static auto throwingCallback() -> Spinner::Callback {
    return []() { throwException<InvalidOperationException>("This is a test exception."); };
  }
};

TEST(SpinnerTest, StartStopTest) {
  Spinner spinner{ TestFixture::trivialCallback() };

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
  spinner.start();

  EXPECT_THROW(spinner.start(), heph::InvalidOperationException);
  spinner.stop().get();

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
}

TEST(SpinnerTest, SpinTest) {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 1 };
  Spinner spinner{ TestFixture::trivialCallback() };
  bool callback_called = false;
  spinner.setTerminationCallback([&callback_called]() { callback_called = true; });

  spinner.start();

  // Wait for a while to let the spinner do some work.
  std::this_thread::sleep_for(WAIT_FOR);
  spinner.stop().get();

  // The counter should have been incremented.
  EXPECT_GT(spinner.spinCount(), 0);
  EXPECT_TRUE(callback_called);
}

TEST(SpinnerTest, Stop) {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::nonThrowingCallback(callback_called_counter));

  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  spinner.stop().get();

  EXPECT_GT(callback_called_counter, 0);
  EXPECT_EQ(callback_called_counter, spinner.spinCount());
}

TEST(SpinnerTest, SpinWithPeriod) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::nonThrowingCallback(callback_called_counter), RATE_HZ);

  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  spinner.stop().get();

  EXPECT_GT(callback_called_counter, 8);
  EXPECT_LT(callback_called_counter, 12);
}

TEST(SpinnerTest, SpinStopsOnStop) {
  static constexpr auto RATE_HZ = 1e3;

  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::stoppingCallback(callback_called_counter), RATE_HZ);

  spinner.start();
  spinner.wait();
  spinner.stop().get();

  EXPECT_EQ(callback_called_counter, 10);
}

TEST(SpinnerTest, ExceptionHandling) {
  static constexpr auto RATE_HZ = 1e3;

  Spinner spinner(TestFixture::throwingCallback(), RATE_HZ);
  bool callback_called = false;
  spinner.setTerminationCallback([&callback_called]() { callback_called = true; });

  spinner.start();
  spinner.wait();
  EXPECT_THROW(spinner.stop().get(), heph::InvalidOperationException);
  EXPECT_TRUE(callback_called);
}

TEST(SpinnerTest, SpinStartAfterStop) {
  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::stoppingCallback(callback_called_counter));

  spinner.start();
  spinner.wait();
  spinner.stop().get();
  EXPECT_EQ(callback_called_counter, 10);

  callback_called_counter = 0;
  spinner.start();
  spinner.wait();
  spinner.stop().get();
  EXPECT_EQ(callback_called_counter, 10);
}

}  // namespace heph::concurrency::tests
