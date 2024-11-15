//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner_state_machine.h"

namespace heph::concurrency {

namspace {
  template <typename Callback>
  struct BinaryTransitionParams {
    State input_state;
    Callback& operation;
    State success_state;
    State failure_state;
  };
  [[nodiscard]] auto attemptBinaryTransition(State current_state, const BinaryTransitionParams& params)
      -> State {
    if (current_state != params.input_state) {
      return current_state;
    }

    try {
      if constexpr (std::is_same_v<decltype(operation), SpinnerCallbacks::TransitionCallback>) {
        params.operation();
        return params.success_state;
      } else if constexpr (std::is_same_v<decltype(operation), SpinnerCallbacks::PolicyCallback>) {
        return params.operation() ? params.success_state : params.failure_state;
      }
    } catch (const std::exception& exception_message) {
      LOG(ERROR) << fmt::format("Spinner state transition failed with message: {}", exception_message);
      return params.failure_state;
    }
  }
}  // namespace

auto createSpinnerStateMachine(SpinnerCallbacks&& callbacks) -> SpinnerStateMachineCallback {
  return [callbacks = std::move(callbacks)](SpinnerState state) mutable -> SpinnerState {
    state = attemptBinaryTransition(state, { .input_state = State::NOT_INITIALIZED,
                                             .operation = callbacks_.init_cb,
                                             .success_state = State::INIT_SUCCESSFUL,
                                             .failure_state = State::FAILED });

    state = attemptBinaryTransition(state, { .input_state = State::READY_TO_SPIN,
                                             .operation = callbacks_.spin_once_cb,
                                             .success_state = State::SPIN_SUCCESSFUL,
                                             .failure_state = State::FAILED });

    state = attemptBinaryTransition(state, { .input_state = State::FAILED,
                                             .operation = callbacks_.shall_restart_cb,
                                             .success_state = State::NOT_INITIALIZED,
                                             .failure_state = State::EXIT });

    state = attemptBinaryTransition(state, { .input_state = State::SPIN_SUCCESSFUL,
                                             .operation = callbacks_.shall_stop_spinning_cb,
                                             .success_state = State::EXIT,
                                             .failure_state = State::READY_TO_SPIN });

    return state;
  };
}

}  // namespace heph::concurrency
