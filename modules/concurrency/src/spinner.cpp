//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <utility>

#include <absl/log/log.h>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency {
namespace {
[[nodiscard]] auto rateToPeriod(double rate_hz) -> std::chrono::microseconds {
  if (rate_hz == 0) {
    return std::chrono::microseconds{ 0 };
  }

  const double period_seconds = 1 / rate_hz;
  const auto period_microseconds = static_cast<uint64_t>(period_seconds * 1e6);

  return std::chrono::microseconds{ period_microseconds };
}

//======= State machine to handle initialization, execution, and stopping of the spinner ==========
enum class State : uint8_t { NOT_INITIALIZED, FAILED, READY_TO_SPIN, SPIN_SUCCESSFUL, EXIT };

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

/// @brief Create a spinner state machine which returns the next state based on the current state.
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
//================================== END OF STATE MACHINE =========================================
}  // namespace

Spinner::Spinner(Callbacks&& callbacks, double rate_hz /*= 0*/)
  : callbacks_(std::move(callbacks)), stop_requested_(false), spin_period_(rateToPeriod(rate_hz)) {
}

Spinner::~Spinner() {
  if (async_spinner_handle_.valid()) {
    LOG(FATAL) << "Spinner is still running. Call stop() before destroying the object.";
    std::terminate();
  }
}

void Spinner::start() {
  throwExceptionIf<InvalidOperationException>(async_spinner_handle_.valid(), "Spinner is already started.");

  stop_requested_.store(false);
  spinner_completed_.clear();
  async_spinner_handle_ = std::async(std::launch::async, [this]() mutable { spin(); });
}

void Spinner::spin() {
  // TODO: set thread name

  start_timestamp_ = std::chrono::system_clock::now();
  uint64_t spin_count_{ 0 };
  auto state = SpinnerState::NOT_INITIALIZED;
  auto state_machine_callback = createSpinnerStateMachine(std::move(callbacks_));

  while (!stop_requested_.load()) {
    state = state_machine_callback(state);
    if (state_ == State::EXIT) {
      break;
    }

    if (spin_period_.count() == 0) {
      continue;
    }
    // Handle periodic spinning at a fixed rate. All operations (initialization, execution etc.) share the
    // same rate.
    const auto target_timestamp = start_timestamp_ + (++spin_count_ * spin_period_.value());
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_until(lock, target_timestamp);
  }

  spinner_completed_.test_and_set();
  spinner_completed_.notify_all();
  termination_callback_();
}

auto Spinner::stop() -> std::future<void> {
  throwExceptionIf<InvalidOperationException>(!async_spinner_handle_.valid(),
                                              "Spinner not yet started, cannot stop.");
  stop_requested_.store(true);
  condition_.notify_all();

  return std::move(async_spinner_handle_);
}

void Spinner::wait() {
  if (!async_spinner_handle_.valid()) {
    return;
  }

  spinner_completed_.wait(false);
}

void Spinner::setTerminationCallback(Callback&& termination_callback) {
  termination_callback_ = std::move(termination_callback);
}

}  // namespace heph::concurrency
