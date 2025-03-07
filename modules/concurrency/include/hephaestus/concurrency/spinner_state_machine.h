//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <functional>

namespace heph::concurrency::spinner_state_machine {

enum class State : std::uint8_t { NOT_INITIALIZED, FAILED, READY_TO_SPIN, SPIN_SUCCESSFUL, EXIT };
enum class Result : std::uint8_t { SUCCESS, FAILURE, REPEAT };

using OperationCallbackT = std::function<Result()>;
using CheckCallbackT = std::function<bool()>;
using StateMachineCallbackT = std::function<State()>;

struct Callbacks {
  OperationCallbackT init_cb = []() -> Result { return Result::SUCCESS; };  //!< Handles initialization.

  OperationCallbackT spin_once_cb = []() -> Result { return Result::SUCCESS; };  //!< Handles execution.

  CheckCallbackT shall_restart_cb = []() -> bool {
    return false;
  };  //!< This callback is called after failure. It decides if operation shall restart, or the spinner
      //!< shall conclude. Default: do not restart.
};

/// @brief Create a callback for the spinner which internally handles the state machine. The current state is
/// stored and a copy is returned to the caller.
[[nodiscard]] auto createStateMachineCallback(Callbacks&& callbacks) -> StateMachineCallbackT;

}  // namespace heph::concurrency::spinner_state_machine
