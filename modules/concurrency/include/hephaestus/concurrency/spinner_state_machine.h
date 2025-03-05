//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <functional>

namespace heph::concurrency::spinner_state_machine {

enum class State : uint8_t { NOT_INITIALIZED, FAILED, READY_TO_SPIN, SPIN_SUCCESSFUL, EXIT };
enum class CallbackResult : uint8_t { SUCCESS, FAILURE };
enum class ExecutionDirective : uint8_t { TRUE, FALSE, REPEAT };

using StateMachineCallbackT = std::function<State()>;
using OperationCallbackT = std::function<CallbackResult()>;
using BinaryCheckCallbackT = std::function<ExecutionDirective()>;

struct Callbacks {
  OperationCallbackT init_cb = []() -> CallbackResult {
    return CallbackResult::SUCCESS;
  };  //!< Handles initialization.

  OperationCallbackT spin_once_cb = []() -> CallbackResult {
    return CallbackResult::SUCCESS;
  };  //!< Handles execution.

  BinaryCheckCallbackT shall_stop_spinning_cb = []() -> ExecutionDirective {
    return ExecutionDirective::FALSE;
  };  //!< This callback is called after successful execution. It decides if spinning shall continue or
      //!< conclude. Default: spin indefinitely.

  BinaryCheckCallbackT shall_restart_cb = []() -> ExecutionDirective {
    return ExecutionDirective::FALSE;
  };  //!< This callback is called after failure. It decides if operation shall restart, or the spinner
      //!< shall conclude. Default: do not restart.
};

/// @brief Create a callback for the spinner which internally handles the state machine. The current state is
/// stored and a copy is returned to the caller.
[[nodiscard]] auto createStateMachineCallback(Callbacks&& callbacks) -> StateMachineCallbackT;

}  // namespace heph::concurrency::spinner_state_machine
