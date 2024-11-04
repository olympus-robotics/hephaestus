//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <stdexcept>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/concurrency/spinner_manager.h"

namespace heph::concurrency::tests {

TEST(SpinnersManager, Empty) {
  SpinnersManager runner_manager{ {} };
  runner_manager.startAll();
  runner_manager.waitAll();
  runner_manager.stopAll();
}

TEST(SpinnersManager, OneSpinnerSuccessful) {
  std::atomic_bool flag = false;
  Spinner spinner{ Spinner::StoppableCallback{ [&flag]() -> Spinner::SpinResult {
    flag = true;
    return Spinner::SpinResult::STOP;
  } } };

  SpinnersManager runner_manager{ { &spinner } };
  runner_manager.startAll();
  runner_manager.waitAll();
  runner_manager.stopAll();

  EXPECT_TRUE(flag);
}

TEST(SpinnersManager, OneSpinnerError) {
  Spinner spinner{ []() { throw std::runtime_error("fail"); } };

  SpinnersManager runner_manager{ { &spinner } };
  runner_manager.startAll();
  runner_manager.waitAll();
  EXPECT_THROW(runner_manager.stopAll(), std::runtime_error);
}

TEST(SpinnersManager, MultipleSpinnersSuccessful) {
  std::atomic_bool flag1 = false;
  Spinner spinner1{ Spinner::StoppableCallback{ [&flag1]() -> Spinner::SpinResult {
    flag1 = true;
    return Spinner::SpinResult::STOP;
  } } };

  std::atomic_bool flag2 = false;
  Spinner spinner2{ Spinner::StoppableCallback{ [&flag2]() -> Spinner::SpinResult {
    flag2 = true;
    return Spinner::SpinResult::STOP;
  } } };

  SpinnersManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  runner_manager.waitAll();
  runner_manager.stopAll();

  EXPECT_TRUE(flag1);
  EXPECT_TRUE(flag2);
}

TEST(SpinnersManager, MultipleSpinnersSuccessfulNoTermination) {
  Spinner spinner1{ []() {} };  // Run indefinitely until stopped
  Spinner spinner2{ []() {} };  // Run indefinitely until stopped
  SpinnersManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
  runner_manager.stopAll();
}

TEST(SpinnersManager, MultipleSpinnersWaitAny) {
  Spinner spinner1{ []() {} };  // Run indefinitely until stopped

  std::atomic_bool flag = false;
  Spinner spinner2{ Spinner::StoppableCallback{ [&flag]() -> Spinner::SpinResult {
    flag = true;
    return Spinner::SpinResult::STOP;
  } } };

  SpinnersManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  runner_manager.waitAny();
  runner_manager.stopAll();

  EXPECT_TRUE(flag);
}

TEST(SpinnersManager, MultipleSpinnersOneError) {
  Spinner spinner1{ []() {} };  // Run indefinitely until stopped

  Spinner spinner2{ []() { throw std::runtime_error("fail"); } };

  SpinnersManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  runner_manager.waitAny();
  EXPECT_THROW(runner_manager.stopAll(), std::runtime_error);
}

}  // namespace heph::concurrency::tests
