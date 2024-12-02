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

[[nodiscard]] auto createStoppingCallback(size_t& callback_called_counter) -> Spinner::StoppableCallback {
  return [&callback_called_counter]() {
    constexpr auto MAX_ITERATION_COUNT = 10;
    if (callback_called_counter < MAX_ITERATION_COUNT) {
      ++callback_called_counter;
      return Spinner::SpinResult::CONTINUE;
    }

    return Spinner::SpinResult::STOP;
  };
}

[[nodiscard]] auto createNonThrowingCallback(size_t& callback_called_counter) -> Spinner::StoppableCallback {
  auto cb = [&callback_called_counter]() { ++callback_called_counter; };
  return Spinner::createNeverStoppingCallback(std::move(cb));
}

[[nodiscard]] auto createThrowingCallback() -> Spinner::StoppableCallback {
  auto cb = []() { throwException<InvalidOperationException>("This is a test exception."); };
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

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
  spinner.start();

  EXPECT_THROW(spinner.start(), heph::InvalidOperationException);
  spinner.stop().get();

  EXPECT_THROW(spinner.stop(), heph::InvalidOperationException);
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
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  auto cb = createNonThrowingCallback(callback_called_counter);
  Spinner spinner{ std::move(cb) };

  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  spinner.stop().get();

  EXPECT_GT(callback_called_counter, 0);
}

TEST(SpinnerTest, SpinWithPeriod) {
  static constexpr auto RATE_HZ = 1e3;
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 10 };

  size_t callback_called_counter = 0;
  auto cb = createNonThrowingCallback(callback_called_counter);
  Spinner spinner{ std::move(cb), RATE_HZ };

  spinner.start();
  std::this_thread::sleep_for(WAIT_FOR);
  spinner.stop().get();

  EXPECT_GT(callback_called_counter, 6);
  EXPECT_LT(callback_called_counter, 12);
}

TEST(SpinnerTest, SpinStopsOnStop) {
  static constexpr auto RATE_HZ = 1e3;

  size_t callback_called_counter = 0;
  auto cb = createStoppingCallback(callback_called_counter);
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
  EXPECT_THROW(spinner.stop().get(), heph::InvalidOperationException);
  EXPECT_TRUE(callback_called);
}

TEST(SpinnerTest, SpinStartAfterStop) {
  size_t callback_called_counter = 0;
  auto cb = createStoppingCallback(callback_called_counter);
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

TEST(SpinnerTest, StateMachine) {
  size_t init_counter = 0;
  size_t successful_spin_counter = 0;
  size_t total_spin_counter = 0;

  // Create a spinner with a state machine. Policies:
  // The spinner will restart indefinitely upon failure.
  // Both init and spin once will fail once, on the first iteration.
  // We thus expect to see three initializations and MAX_ITERATION_COUNT spins.
  auto callbacks = Spinner::StateMachineCallbacks{
    .init_cb =
        [&init_counter]() {
          {
            ++init_counter;
            throwExceptionIf<InvalidOperationException>(init_counter == 1, "Init failed.");
          }
        },
    .spin_once_cb =
        [&successful_spin_counter, &total_spin_counter]() {
          ++total_spin_counter;
          throwExceptionIf<InvalidOperationException>(total_spin_counter == 1, "Spin failed.");
          ++successful_spin_counter;
        },
    .shall_stop_spinning_cb =
        [&successful_spin_counter]() { return successful_spin_counter < MAX_ITERATION_COUNT ? false : true; },
    .shall_restart_cb = []() { return true; }
  };

  auto cb = Spinner::createCallbackWithStateMachine(std::move(callbacks));
  Spinner spinner(std::move(cb));

  spinner.start();
  spinner.wait();
  spinner.stop().get();

  EXPECT_EQ(init_counter, 3);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  EXPECT_EQ(successful_spin_counter, MAX_ITERATION_COUNT);
  EXPECT_EQ(total_spin_counter, MAX_ITERATION_COUNT + 1);
}

}  // namespace heph::concurrency::tests
