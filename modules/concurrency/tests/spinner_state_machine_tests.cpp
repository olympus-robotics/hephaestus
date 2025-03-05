//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner_state_machine.h"

namespace heph::concurrency::spinner_state_machine::tests {

TEST(SpinnerStateMachineTest, TestCallback) {
  static constexpr std::size_t MAX_ITERATION_COUNT = 10ul;

  std::size_t init_counter = 0;
  std::size_t successful_spin_counter = 0;
  std::size_t total_spin_counter = 0;
  std::size_t shall_stop_spinning_repeat_counter = 0;
  std::size_t state_machine_counter = 0;

  // Create a spinner state machine. Policies:
  // The state machine will restart indefinitely upon failure.
  // Both init and spin once will fail once, on the first iteration.
  // We thus expect to see three initializations and MAX_ITERATION_COUNT spins.
  // A state_machine_counter is used to verify that each callback is called once per iteration.
  auto callbacks =
      Callbacks{ .init_cb = [&init_counter, &state_machine_counter]() -> CallbackResult {
                  ++state_machine_counter;
                  ++init_counter;
                  if (init_counter == 1) {
                    return CallbackResult::FAILURE;
                  }
                  return CallbackResult::SUCCESS;
                },

                 .spin_once_cb = [&successful_spin_counter, &total_spin_counter,
                                  &state_machine_counter]() -> CallbackResult {
                   ++state_machine_counter;
                   ++total_spin_counter;
                   if (total_spin_counter == 1) {
                     return CallbackResult::FAILURE;
                   }
                   ++successful_spin_counter;
                   return CallbackResult::SUCCESS;
                 },

                 .shall_stop_spinning_cb = [&successful_spin_counter, &shall_stop_spinning_repeat_counter,
                                            &state_machine_counter]() -> ExecutionDirective {
                   ++state_machine_counter;
                   if (shall_stop_spinning_repeat_counter < 1) {
                     ++shall_stop_spinning_repeat_counter;
                     return ExecutionDirective::REPEAT;
                   }
                   return successful_spin_counter < MAX_ITERATION_COUNT ? ExecutionDirective::FALSE :
                                                                          ExecutionDirective::TRUE;
                 },

                 .shall_restart_cb = [&state_machine_counter]() -> ExecutionDirective {
                   ++state_machine_counter;
                   return ExecutionDirective::TRUE;
                 } };

  auto callback = createStateMachineCallback(std::move(callbacks));

  auto state = State{};
  static constexpr auto LOOP_BOUND = 10 * MAX_ITERATION_COUNT;
  std::size_t callback_counter = 0;
  for (; state != State::EXIT && callback_counter < LOOP_BOUND; ++callback_counter) {
    state = callback();
  }
  EXPECT_EQ(state, State::EXIT);

  EXPECT_EQ(init_counter, 3);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  EXPECT_EQ(shall_stop_spinning_repeat_counter, 1);
  EXPECT_EQ(successful_spin_counter, MAX_ITERATION_COUNT);
  EXPECT_EQ(total_spin_counter, MAX_ITERATION_COUNT + 1);
  EXPECT_EQ(state_machine_counter, callback_counter);
}

}  // namespace heph::concurrency::spinner_state_machine::tests
