//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner_state_machine.h"

namespace heph::concurrency::spinner_state_machine::tests {

TEST(SpinnerStateMachineTest, TestCallback) {
  static constexpr std::size_t MAX_ITERATION_COUNT = 10ul;

  std::size_t init_counter = 0;
  std::size_t init_repeat_counter = 0;
  std::size_t spin_once_repeat_counter = 0;
  std::size_t total_spin_once_counter = 0;
  std::size_t state_machine_counter = 0;

  // Create a spinner state machine. Policies:
  // The state machine will restart indefinitely upon failure.
  // Both init and spin once will fail once, on the first iteration.
  // We thus expect to see three initializations and MAX_ITERATION_COUNT spins.
  // A state_machine_counter is used to verify that each callback is called once per iteration.
  auto callbacks =
      Callbacks{ .init_cb = [&init_counter, &state_machine_counter, &init_repeat_counter]() -> Result {
                  ++state_machine_counter;
                  ++init_counter;
                  if (init_counter == 1) {
                    return Result::FAILURE;
                  }
                  if (init_counter == 2) {
                    ++init_repeat_counter;
                    return Result::REPEAT;
                  }
                  return Result::PROCEED;
                },

                 .spin_once_cb = [&spin_once_repeat_counter, &total_spin_once_counter,
                                  &state_machine_counter]() -> Result {
                   ++state_machine_counter;
                   ++total_spin_once_counter;
                   if (total_spin_once_counter == 1) {
                     return Result::FAILURE;
                   }
                   ++spin_once_repeat_counter;
                   return spin_once_repeat_counter < MAX_ITERATION_COUNT ? Result::REPEAT : Result::PROCEED;
                 },

                 .shall_restart_cb = []() -> bool { return true; } };

  auto callback = createStateMachineCallback(std::move(callbacks));

  auto state = State{};
  static constexpr auto LOOP_BOUND = 10 * MAX_ITERATION_COUNT;
  std::size_t callback_counter = 0;
  for (; state != State::EXIT && callback_counter < LOOP_BOUND; ++callback_counter) {
    state = callback();
  }
  EXPECT_EQ(state, State::EXIT);

  EXPECT_EQ(init_counter, 3 + init_repeat_counter);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  EXPECT_EQ(init_repeat_counter, 1);
  EXPECT_EQ(spin_once_repeat_counter, MAX_ITERATION_COUNT);
  EXPECT_EQ(total_spin_once_counter, spin_once_repeat_counter + 1);
  EXPECT_EQ(state_machine_counter, callback_counter);
}

}  // namespace heph::concurrency::spinner_state_machine::tests
