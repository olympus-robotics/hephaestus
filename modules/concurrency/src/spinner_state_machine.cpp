//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner_state_machine.h"

#include <utility>

namespace heph::concurrency::spinner_state_machine {

namespace {
struct OperationParams {
  OperationCallbackT& operation;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  State proceed_state;
  State failure_state;
};

[[nodiscard]] auto executeOperation(State current_state, const OperationParams& params) -> State {
  const auto result = params.operation();

  switch (result) {
    case Result::PROCEED:
      return params.proceed_state;
    case Result::FAILURE:
      return params.failure_state;
    case Result::REPEAT:
      return current_state;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable.
}
}  // namespace

auto createStateMachineCallback(Callbacks&& callbacks) -> StateMachineCallbackT {
  return [callbacks = std::move(callbacks), state = State::NOT_INITIALIZED]() mutable -> State {
    // Ensure that either init or spin once is called in one spin iteration, but not both.
    if (state == State::NOT_INITIALIZED) {
      state = executeOperation(state, OperationParams{ .operation = callbacks.init_cb,
                                                       .proceed_state = State::READY_TO_SPIN,
                                                       .failure_state = State::FAILED });
    } else if (state == State::READY_TO_SPIN) {
      state = executeOperation(state, OperationParams{ .operation = callbacks.spin_once_cb,
                                                       .proceed_state = State::EXIT,
                                                       .failure_state = State::FAILED });
    }

    // Shall restart is called after failure, within the same spin iteration.
    if (state == State::FAILED) {
      state = callbacks.shall_restart_cb() ? State::NOT_INITIALIZED : State::EXIT;
    }

    return state;
  };
}

}  // namespace heph::concurrency::spinner_state_machine
