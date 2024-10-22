//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstddef>
#include <functional>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::tests {

struct TestFixture : public ::testing::Test {
  static auto trivialCallback() -> Spinner::Callback {
    return []() { return Spinner::SpinResult::Continue; };
  }

  static auto stoppingCallback(size_t& callback_called_counter) -> Spinner::Callback {
    return [&callback_called_counter]() {
      static constexpr auto STOP_AFTER = 10;
      if (callback_called_counter < STOP_AFTER) {
        ++callback_called_counter;
        return Spinner::SpinResult::Continue;
      }

      return Spinner::SpinResult::Stop;
    };
  }

  static auto nonThrowingCallback(size_t& callback_called_counter) -> Spinner::Callback {
    return [&callback_called_counter]() {
      ++callback_called_counter;
      return Spinner::SpinResult::Continue;
    };
  }

  static auto throwingCallback() -> Spinner::Callback {
    return []() {
      throwException<InvalidOperationException>("This is a test exception.");
      return Spinner::SpinResult::Continue;
    };
  }
};

TEST(SpinnerTest, StartStopTest) {
  Spinner spinner{ TestFixture::trivialCallback() };

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
  spinner.start();

  EXPECT_THROW(spinner.start(), heph::InvalidOperationException);
  auto future = spinner.stop();
  future.get();

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
}

TEST(SpinnerTest, SpinTest) {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 1 };
  Spinner spinner{ TestFixture::trivialCallback() };

  spinner.start();

  // Wait for a while to let the spinner do some work.
  std::this_thread::sleep_for(WAIT_FOR);
  auto future = spinner.stop();
  future.get();

  // The counter should have been incremented.
  EXPECT_GT(spinner.spinCount(), 0);
}

TEST(SpinnerTest, StopCallback) {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::nonThrowingCallback(callback_called_counter));

  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  auto future = spinner.stop();
  future.get();

  EXPECT_GT(callback_called_counter, 0);
  EXPECT_EQ(callback_called_counter, spinner.spinCount());
}

TEST(SpinnerTest, SpinWithPeriod) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::nonThrowingCallback(callback_called_counter), RATE_HZ);

  try {
    spinner.start();
    std::this_thread::sleep_for(WAIT_FOR);
    spinner.stop().get();

    EXPECT_GT(callback_called_counter, 8);
    EXPECT_LT(callback_called_counter, 12);
  } catch (const std::exception& e) {
    EXPECT_TRUE(false) << "Spinner must not throw when passed a nonThrowingCallback" << e.what();
  }
}

TEST(SpinnerTest, SpinStopsOnStop) {
  static constexpr auto RATE_HZ = 1e3;

  size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::stoppingCallback(callback_called_counter), RATE_HZ);

  spinner.start();
  spinner.wait();

  EXPECT_EQ(callback_called_counter, 10);
}

TEST(SpinnerTest, ExceptionHandling) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 5 };

  Spinner spinner(TestFixture::throwingCallback(), RATE_HZ);
  try {
    spinner.start();
    std::this_thread::sleep_for(WAIT_FOR);
    spinner.stop().get();
    EXPECT_TRUE(false) << "Spinner must throw when passed a ThrowingCallback";
  } catch (const std::exception& e) {
    EXPECT_TRUE(true) << "Spinner must throw when passed a ThrowingCallback" << e.what();
  }
}

}  // namespace heph::concurrency::tests
