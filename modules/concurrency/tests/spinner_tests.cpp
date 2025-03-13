//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::tests {

namespace {
constexpr auto MAX_ITERATION_COUNT = 10;

[[nodiscard]] auto createTrivialCallback() -> Spinner::StoppableCallback {
  auto cb = []() {};
  return Spinner::createNeverStoppingCallback(std::move(cb));
}

[[nodiscard]] auto createSelfStoppingCallback(size_t& callback_called_counter) -> Spinner::StoppableCallback {
  return [&callback_called_counter]() {
    constexpr auto MAX_ITERATION_COUNT = 10;
    if (callback_called_counter < MAX_ITERATION_COUNT) {
      ++callback_called_counter;
      return Spinner::SpinResult::CONTINUE;
    }

    return Spinner::SpinResult::STOP;
  };
}

[[nodiscard]] auto createNonThrowingCallback(size_t& callback_called_counter, std::atomic_flag& flag)
    -> Spinner::StoppableCallback {
  auto cb = [&callback_called_counter, &flag]() {
    ++callback_called_counter;
    flag.test_and_set();
    flag.notify_all();
  };
  return Spinner::createNeverStoppingCallback(std::move(cb));
}

[[nodiscard]] auto createThrowingCallback() -> Spinner::StoppableCallback {
  auto cb = []() { panic("This is a test exception.", std::source_location::current()); };
  return Spinner::createNeverStoppingCallback(std::move(cb));
}
}  // namespace

TEST(SpinnerTest, ComputeNextSpinTimestamp) {
  using ClockT = std::chrono::system_clock;
  const auto start_timestamp = ClockT::time_point{ std::chrono::milliseconds{ 0 } };
  const auto spin_period = std::chrono::milliseconds{ 10 };
  {
    const auto now = start_timestamp + std::chrono::milliseconds{ 5 };
    const auto expected_next_spin_timestamp = ClockT::time_point{ std::chrono::milliseconds{ 10 } };
    const auto next_spin_timestamp = internal::computeNextSpinTimestamp(start_timestamp, now, spin_period);
    EXPECT_EQ(next_spin_timestamp, expected_next_spin_timestamp);
  }

  {
    const auto now = start_timestamp + std::chrono::milliseconds{ 12 };
    const auto expected_next_spin_timestamp = ClockT::time_point{ std::chrono::milliseconds{ 20 } };
    const auto next_spin_timestamp = internal::computeNextSpinTimestamp(start_timestamp, now, spin_period);
    EXPECT_EQ(next_spin_timestamp, expected_next_spin_timestamp);
  }

  {
    const auto now = start_timestamp + std::chrono::milliseconds{ 49 };
    const auto expected_next_spin_timestamp = ClockT::time_point{ std::chrono::milliseconds{ 50 } };
    const auto next_spin_timestamp = internal::computeNextSpinTimestamp(start_timestamp, now, spin_period);
    EXPECT_EQ(next_spin_timestamp, expected_next_spin_timestamp);
  }

  {
    const auto now = start_timestamp + std::chrono::milliseconds{ 50 };
    const auto expected_next_spin_timestamp = ClockT::time_point{ std::chrono::milliseconds{ 50 } };
    const auto next_spin_timestamp = internal::computeNextSpinTimestamp(start_timestamp, now, spin_period);
    EXPECT_EQ(next_spin_timestamp, expected_next_spin_timestamp);
  }
}

TEST(SpinnerTest, StartStopTest) {
  auto cb = createTrivialCallback();
  Spinner spinner{ std::move(cb) };

  EXPECT_THROW_OR_DEATH(spinner.stop(), heph::Panic, "");
  spinner.start();

  EXPECT_THROW_OR_DEATH(spinner.start(), heph::Panic, "");
  spinner.stop().get();

  EXPECT_THROW_OR_DEATH(spinner.stop(), heph::Panic, "");
}

TEST(SpinnerTest, SpinTest) {
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  Spinner spinner{ Spinner::createNeverStoppingCallback([&flag]() {
    flag.test_and_set();
    flag.notify_all();
  }) };
  bool callback_called = false;
  spinner.setTerminationCallback([&callback_called]() { callback_called = true; });

  spinner.start();

  // Wait for a while to let the spinner do some work.
  flag.wait(false);
  spinner.stop().get();

  // The counter should have been incremented.
  EXPECT_TRUE(callback_called);
}

TEST(SpinnerTest, Stop) {
  size_t callback_called_counter = 0;
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  auto cb = createNonThrowingCallback(callback_called_counter, flag);
  Spinner spinner{ std::move(cb) };

  spinner.start();
  flag.wait(false);
  spinner.stop().get();

  EXPECT_GT(callback_called_counter, 0);
}

TEST(SpinnerTest, SpinWithPeriod) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  auto cb = createNonThrowingCallback(callback_called_counter, flag);
  Spinner spinner{ std::move(cb), RATE_HZ };

  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  flag.wait(false);
  spinner.stop().get();

  EXPECT_GT(callback_called_counter, 0);
  EXPECT_LT(callback_called_counter, 20);
}

TEST(SpinnerTest, SpinStopsOnStop) {
  static constexpr auto RATE_HZ = 1e3;

  size_t callback_called_counter = 0;
  auto cb = createSelfStoppingCallback(callback_called_counter);
  Spinner spinner(std::move(cb), RATE_HZ);

  spinner.start();
  spinner.wait();
  spinner.stop().get();

  EXPECT_EQ(callback_called_counter, MAX_ITERATION_COUNT);
}

TEST(SpinnerTest, ExceptionHandling) {
  static constexpr auto RATE_HZ = 1e3;

  auto cb = createThrowingCallback();
  Spinner spinner(std::move(cb), RATE_HZ);
  bool callback_called = false;
  spinner.setTerminationCallback([&callback_called]() { callback_called = true; });

  spinner.start();
  spinner.wait();
  EXPECT_THROW(spinner.stop().get(), heph::Panic);
  EXPECT_TRUE(callback_called);
}

TEST(SpinnerTest, SpinStartAfterStop) {
  size_t callback_called_counter = 0;
  auto cb = createSelfStoppingCallback(callback_called_counter);
  Spinner spinner(std::move(cb));

  spinner.start();
  spinner.wait();
  spinner.stop().get();
  EXPECT_EQ(callback_called_counter, MAX_ITERATION_COUNT);

  callback_called_counter = 0;
  spinner.start();
  spinner.wait();
  spinner.stop().get();
  EXPECT_EQ(callback_called_counter, MAX_ITERATION_COUNT);
}

}  // namespace heph::concurrency::tests
