//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <stdexcept>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/concurrency/spinner_manager.h"

namespace heph::concurrency::tests {

TEST(RunnerManager, Empty) {
  SpinnerManager runner_manager{ {} };
  runner_manager.startAll();
  runner_manager.waitAll();
  runner_manager.stopAll();
}

TEST(RunnerManager, OneSpinnerSuccessful) {
  std::atomic_bool flag = false;
  Spinner spinner{ Spinner::StoppableCallback{ [&flag]() -> Spinner::SpinResult {
    flag = true;
    return Spinner::SpinResult::STOP;
  } } };

  SpinnerManager runner_manager{ { &spinner } };
  runner_manager.startAll();
  runner_manager.waitAll();
  runner_manager.stopAll();

  EXPECT_TRUE(flag);
}

TEST(RunnerManager, OneSpinnerError) {
  Spinner spinner{ []() { throw std::runtime_error("fail"); } };

  SpinnerManager runner_manager{ { &spinner } };
  runner_manager.startAll();
  runner_manager.waitAll();
  EXPECT_THROW(runner_manager.stopAll(), std::runtime_error);
}

TEST(RunnerManager, MultipleSpinnersSuccessful) {
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

  SpinnerManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  runner_manager.waitAll();
  runner_manager.stopAll();

  EXPECT_TRUE(flag1);
  EXPECT_TRUE(flag2);
}

TEST(RunnerManager, MultipleSpinnersWaitAny) {
  Spinner spinner1{ []() {} };  // Run indefinitely until stopped

  std::atomic_bool flag = false;
  Spinner spinner2{ Spinner::StoppableCallback{ [&flag]() -> Spinner::SpinResult {
    flag = true;
    return Spinner::SpinResult::STOP;
  } } };

  SpinnerManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  runner_manager.waitAny();
  runner_manager.stopAll();

  EXPECT_TRUE(flag);
}

TEST(RunnerManager, MultipleSpinnersOneError) {
  Spinner spinner1{ []() {} };  // Run indefinitely until stopped

  Spinner spinner2{ []() { throw std::runtime_error("fail"); } };

  SpinnerManager runner_manager{ { &spinner1, &spinner2 } };
  runner_manager.startAll();
  runner_manager.waitAny();
  EXPECT_THROW(runner_manager.stopAll(), std::runtime_error);
}

}  // namespace heph::concurrency::tests
