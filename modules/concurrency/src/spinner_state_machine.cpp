//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner_state_machine.h"

#include <utility>

namespace heph::concurrency::spinner_state_machine {

namespace {
struct OperationParams {
  OperationCallbackT& operation;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  State success_state;
  State failure_state;
};

[[nodiscard]] auto executeOperation(State current_state, const OperationParams& params) -> State {
  const auto result = params.operation();

  switch (result) {
    case Result::SUCCESS:
      return params.success_state;
    case Result::FAILURE:
      return params.failure_state;
    case Result::REPEAT:
      return current_state;
  }
}
}  // namespace

auto createStateMachineCallback(Callbacks&& callbacks) -> StateMachineCallbackT {
  auto execute_state_transition = [callbacks = std::move(callbacks)](State state) -> State {
    switch (state) {
      case State::NOT_INITIALIZED:
        return executeOperation(state, OperationParams{ .operation = callbacks.init_cb,
                                                        .success_state = State::READY_TO_SPIN,
                                                        .failure_state = State::FAILED });
      case State::READY_TO_SPIN:
        return executeOperation(state, OperationParams{ .operation = callbacks.spin_once_cb,
                                                        .success_state = State::SPIN_SUCCESSFUL,
                                                        .failure_state = State::FAILED });
      case State::FAILED:
        return callbacks.shall_restart_cb() ? State::NOT_INITIALIZED : State::EXIT;
      default:
        return state;
    }
  };

  return [&execute_state_transition, state = State::NOT_INITIALIZED]() mutable -> State {
    state = execute_state_transition(state);
    return state;
  };
}

}  // namespace heph::concurrency::spinner_state_machine
