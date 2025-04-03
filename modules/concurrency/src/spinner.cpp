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
#include <utility>

#include "hephaestus/concurrency/spinner_state_machine.h"
#include "hephaestus/telemetry/log.h"
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
}  // namespace

auto Spinner::createNeverStoppingCallback(Callback&& callback) -> StoppableCallback {
  return [callback = std::move(callback)]() -> SpinResult {
    callback();
    return SpinResult::CONTINUE;
  };
}

auto Spinner::createCallbackWithStateMachine(
    spinner_state_machine::StateMachineCallbackT&& state_machine_callback) -> StoppableCallback {
  return [state_machine_callback = std::move(state_machine_callback)]() mutable -> SpinResult {
    const auto state = state_machine_callback();

    // If the state machine reaches the exit state, the spinner should stop, else continue spinning.
    if (state == spinner_state_machine::State::EXIT) {
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
    heph::log(heph::FATAL, "Spinner is still running. Call stop() before destroying the object.");
    std::terminate();
  }
}

void Spinner::start() {
  panicIf(async_spinner_handle_.valid(), "Spinner is already started.");

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
      heph::log(heph::ERROR, "Spinner caught an exception, terminating", "error", e.what());
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
  panicIf(!async_spinner_handle_.valid(), "Spinner not yet started, cannot stop.");
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
                              std::chrono::duration<double> spin_period)
    -> std::chrono::system_clock::time_point {
  const auto duration_since_start =
      std::chrono::duration_cast<std::chrono::duration<double>>(now - start_timestamp);
  const auto spin_count =
      static_cast<std::size_t>(std::ceil(duration_since_start.count() / spin_period.count()));

  return start_timestamp + (spin_count * spin_period);
}
}  // namespace internal
}  // namespace heph::concurrency
