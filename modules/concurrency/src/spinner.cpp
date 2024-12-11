//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <limits>
#include <mutex>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency {
namespace {
[[nodiscard]] auto rateToPeriod(double rate_hz) -> std::chrono::microseconds {
  // Explicit check to prevent floating point errors
  if (rate_hz == std::numeric_limits<double>::infinity()) {
    return std::chrono::microseconds{ 0 };
  }

  const double period_seconds = 1 / rate_hz;
  const auto period_microseconds = static_cast<uint64_t>(period_seconds * 1e6);

  return std::chrono::microseconds{ period_microseconds };
}

enum class State : uint8_t { NOT_INITIALIZED, FAILED, READY_TO_SPIN, SPIN_SUCCESSFUL, EXIT };

template <typename CallbackT>
struct BinaryTransitionParams {
  State input_state;
  CallbackT& operation;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  State success_state;
  State failure_state;
};

template <typename T>
struct IsPolicyCallback : std::false_type {};

template <>
struct IsPolicyCallback<Spinner::StateMachineCallbacks::PolicyCallback> : std::true_type {};

template <typename CallbackT>
[[nodiscard]] auto attemptBinaryTransition(State current_state,
                                           const BinaryTransitionParams<CallbackT>& params) -> State {
  if (current_state != params.input_state) {
    return current_state;
  }

  try {
    if constexpr (IsPolicyCallback<CallbackT>::value) {
      return params.operation() ? params.success_state : params.failure_state;
    }

    params.operation();
  } catch (const std::exception& exception_message) {
    // heph::log(heph::ERROR, "spinner state transition failed", "exception", exception_message.what());

    return params.failure_state;
  }

  return params.success_state;
}
}  // namespace

auto Spinner::createNeverStoppingCallback(Callback&& callback) -> StoppableCallback {
  return [callback = std::move(callback)]() -> SpinResult {
    callback();
    return SpinResult::CONTINUE;
  };
}

auto Spinner::createCallbackWithStateMachine(StateMachineCallbacks&& callbacks) -> StoppableCallback {
  return [callbacks = std::move(callbacks), state = State::NOT_INITIALIZED]() mutable -> SpinResult {
    state = attemptBinaryTransition(state, BinaryTransitionParams{ .input_state = State::NOT_INITIALIZED,
                                                                   .operation = callbacks.init_cb,
                                                                   .success_state = State::READY_TO_SPIN,
                                                                   .failure_state = State::FAILED });

    state = attemptBinaryTransition(state, BinaryTransitionParams{ .input_state = State::READY_TO_SPIN,
                                                                   .operation = callbacks.spin_once_cb,
                                                                   .success_state = State::SPIN_SUCCESSFUL,
                                                                   .failure_state = State::FAILED });

    state = attemptBinaryTransition(state, BinaryTransitionParams{ .input_state = State::FAILED,
                                                                   .operation = callbacks.shall_restart_cb,
                                                                   .success_state = State::NOT_INITIALIZED,
                                                                   .failure_state = State::EXIT });

    state =
        attemptBinaryTransition(state, BinaryTransitionParams{ .input_state = State::SPIN_SUCCESSFUL,
                                                               .operation = callbacks.shall_stop_spinning_cb,
                                                               .success_state = State::EXIT,
                                                               .failure_state = State::READY_TO_SPIN });

    // If the state machine reaches the exit state, the spinner should stop, else continue spinning.
    if (state == State::EXIT) {
      return SpinResult::STOP;
    }

    return SpinResult::CONTINUE;
  };
}

Spinner::Spinner(StoppableCallback&& stoppable_callback,
                 double rate_hz /*= std::numeric_limits<double>::infinity()*/)
  : stoppable_callback_(std::move(stoppable_callback))
  , stop_requested_(false)
  , spin_period_(rateToPeriod(rate_hz)) {
}

Spinner::~Spinner() {
  if (async_spinner_handle_.valid()) {
    // heph::log(heph::FATAL, "Spinner is still running. Call stop() before destroying the object.");
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
  // TODO(@brizzi): set thread name

  auto start_timestamp = std::chrono::system_clock::now();
  while (!stop_requested_.load()) {
    try {
      if (stoppable_callback_() == SpinResult::STOP) {
        break;
      }

      if (spin_period_.count() > 0) {  // Throttle spinner to a fixed rate if a valid rate_hz is provided.
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_until(lock, internal::computeNextSpinTimestamp(
                                        start_timestamp, std::chrono::system_clock::now(), spin_period_));
      }
    } catch (std::exception& e) {
      terminate();
      throw;
    }
  }

  terminate();
}

void Spinner::terminate() {
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

namespace internal {
auto computeNextSpinTimestamp(const std::chrono::system_clock::time_point& start_timestamp,
                              const std::chrono::system_clock::time_point& now,
                              const std::chrono::microseconds& spin_period)
    -> std::chrono::system_clock::time_point {
  const auto elapsed_time_since_start_micro_sec = static_cast<double>(
      std::chrono::duration_cast<std::chrono::microseconds>(now - start_timestamp).count());
  const auto spin_count = static_cast<std::size_t>(
      std::ceil(elapsed_time_since_start_micro_sec / static_cast<double>(spin_period.count())));
  return start_timestamp + (spin_count * spin_period);
}
}  // namespace internal
}  // namespace heph::concurrency
