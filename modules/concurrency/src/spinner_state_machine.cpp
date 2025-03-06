//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner_state_machine.h"

#include <utility>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency::spinner_state_machine {

namespace {
struct OperationParams {
  OperationCallbackT& operation;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  State success_state;
  State failure_state;
};

[[nodiscard]] auto executeOperation(const OperationParams& params) -> State {
  const auto result = params.operation();

  switch (result) {
    case CallbackResult::SUCCESS:
      return params.success_state;
    case CallbackResult::FAILURE:
      return params.failure_state;
  }

  heph::throwException<heph::InvalidParameterException>("Invalid callback result");
}

struct BinaryCheckParams {
  BinaryCheckCallbackT& check;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  State true_state;
  State false_state;
  State repeat_state;
};

[[nodiscard]] auto executeBinaryCheck(const BinaryCheckParams& params) -> State {
  const auto result = params.check();
  switch (result) {
    case ExecutionDirective::TRUE:
      return params.true_state;
    case ExecutionDirective::FALSE:
      return params.false_state;
    case ExecutionDirective::REPEAT:
      return params.repeat_state;
  }

  heph::throwException<heph::InvalidParameterException>("Invalid callback result");
}
}  // namespace

auto createStateMachineCallback(Callbacks&& callbacks) -> StateMachineCallbackT {
  return [callbacks = std::move(callbacks), state = State::NOT_INITIALIZED]() mutable -> State {
    switch (state) {
      case State::NOT_INITIALIZED:
        state = executeOperation(OperationParams{ .operation = callbacks.init_cb,
                                                  .success_state = State::READY_TO_SPIN,
                                                  .failure_state = State::FAILED });
        break;
      case State::READY_TO_SPIN:
        state = executeOperation(OperationParams{ .operation = callbacks.spin_once_cb,
                                                  .success_state = State::SPIN_SUCCESSFUL,
                                                  .failure_state = State::FAILED });
        break;
      case State::FAILED:
        state = executeBinaryCheck(BinaryCheckParams{ .check = callbacks.shall_restart_cb,
                                                      .true_state = State::NOT_INITIALIZED,
                                                      .false_state = State::EXIT,
                                                      .repeat_state = State::FAILED });
        break;
      case State::SPIN_SUCCESSFUL:
        state = executeBinaryCheck(BinaryCheckParams{ .check = callbacks.shall_stop_spinning_cb,
                                                      .true_state = State::EXIT,
                                                      .false_state = State::READY_TO_SPIN,
                                                      .repeat_state = State::SPIN_SUCCESSFUL });
        break;
      default:
        state = State::EXIT;
        break;
    }

    return state;
  };
}

}  // namespace heph::concurrency::spinner_state_machine
