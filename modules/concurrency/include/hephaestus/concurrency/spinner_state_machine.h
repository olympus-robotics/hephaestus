//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

namespace heph::concurrency {

struct SpinnerCallbacks {
using TransitionCallback = std::function<void()>;
using PolicyCallback = std::function<bool()>;

TransitionCallback init_cb = []() {};
TransitionCallback spin_once_cb = []() {};
TransitionCallback completion_cb = []() {};

PolicyCallback shall_stop_spinning_cb = []() { return false; };  //!< Default: spin indefinitely.
PolicyCallback shall_restart_cb = []() { return false; };        //!< Default: do not restart.
};

enum class SpinnerState : uint8_t { NOT_INITIALIZED, FAILED, READY_TO_SPIN, SPIN_SUCCESSFUL, EXIT };

using SpinnerStateMachineCallback = std::function<SpinnerState(SpinnerState)>;

/// @brief Create a spinner state machine which returns the next state based on the current state.
[[nodiscard]] auto createSpinnerStateMachine(SpinnerCallbacks&& callbacks) -> SpinnerStateMachineCallback;


}  // namespace heph::concurrency
