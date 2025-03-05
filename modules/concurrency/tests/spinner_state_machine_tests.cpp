//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"

namespace heph::concurrency::tests {

// TEST(SpinnerTest, StateMachine) {
//   size_t init_counter = 0;
//   size_t successful_spin_counter = 0;
//   size_t total_spin_counter = 0;

//   // Create a spinner with a state machine. Policies:
//   // The spinner will restart indefinitely upon failure.
//   // Both init and spin once will fail once, on the first iteration.
//   // We thus expect to see three initializations and MAX_ITERATION_COUNT spins.
//   auto callbacks = Spinner::StateMachineCallbacks{
//     .init_cb =
//         [&init_counter]() {
//           {
//             ++init_counter;
//             if (init_counter == 1) [[unlikely]] {
//               throw InvalidOperationException{ "Init failed.", std::source_location::current() };
//             }
//           }
//         },
//     .spin_once_cb =
//         [&successful_spin_counter, &total_spin_counter]() {
//           ++total_spin_counter;
//           if (total_spin_counter == 1) [[unlikely]] {
//             throw InvalidOperationException{ "Spin failed.", std::source_location::current() };
//           }
//           ++successful_spin_counter;
//         },
//     .shall_stop_spinning_cb =
//         [&successful_spin_counter]() { return successful_spin_counter < MAX_ITERATION_COUNT ? false : true;
//         },
//     .shall_restart_cb = []() { return true; }
//   };

//   auto cb = Spinner::createCallbackWithStateMachine(std::move(callbacks));
//   Spinner spinner(std::move(cb));

//   spinner.start();
//   spinner.wait();
//   spinner.stop().get();

//   EXPECT_EQ(init_counter, 3);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
//   EXPECT_EQ(successful_spin_counter, MAX_ITERATION_COUNT);
//   EXPECT_EQ(total_spin_counter, MAX_ITERATION_COUNT + 1);
// }

}  // namespace heph::concurrency::tests
