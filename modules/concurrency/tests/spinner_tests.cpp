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
  static auto TrivialCallback() -> std::function<Spinner::SpinResult()> {
    return []() { return Spinner::SpinResult::Continue; };
  }

  static auto NonThrowingCallback(size_t& callback_called_counter) -> std::function<Spinner::SpinResult()> {
    return [&callback_called_counter]() {
      ++callback_called_counter;
      return Spinner::SpinResult::Continue;
    };
  }

  static auto ThrowingCallback() -> std::function<Spinner::SpinResult()> {
    return []() {
      throwException<InvalidOperationException>("This is a test exception.");
      return Spinner::SpinResult::Continue;
    };
  }
};

TEST(SpinnerTest, StartStopTest) {
  Spinner spinner{ TestFixture::TrivialCallback() };

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
  spinner.start();

  EXPECT_THROW(spinner.start(), heph::InvalidOperationException);
  auto future = spinner.stop();
  future.get();

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
}

TEST(SpinnerTest, SpinTest) {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 1 };
  Spinner spinner{ TestFixture::TrivialCallback() };

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
  Spinner spinner(TestFixture::NonThrowingCallback(callback_called_counter));

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

  std::size_t callback_called_counter = 0;
  Spinner spinner(TestFixture::NonThrowingCallback(callback_called_counter), RATE_HZ);

  try {
    spinner.start();
    std::this_thread::sleep_for(WAIT_FOR);
    spinner.stop().get();

    EXPECT_GT(callback_called_counter, 8);
    EXPECT_LT(callback_called_counter, 12);
  } catch (const std::exception& e) {
    ASSERT_TRUE(false) << "Spinner must not throw when passed a NonThrowingCallback" << e.what();
  }
}

TEST(SpinnerTest, ExceptionHandling) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 5 };

  Spinner spinner(TestFixture::ThrowingCallback(), RATE_HZ);
  try {
    spinner.start();
    std::this_thread::sleep_for(WAIT_FOR);
    spinner.stop().get();
    ASSERT_TRUE(false) << "Spinner must throw when passed a ThrowingCallback";
  } catch (const std::exception& e) {
    ASSERT_TRUE(true) << "Spinner must throw when passed a ThrowingCallback" << e.what();
  }
}

}  // namespace heph::concurrency::tests
