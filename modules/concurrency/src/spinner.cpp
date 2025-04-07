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
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/utils/timing/stop_watch.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency {

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
                 std::optional<std::chrono::duration<double>> spin_period /*= std::nullopt*/,
                 std::optional<std::string> component_name /*= std::nullopt*/)
  : component_name_(std::move(component_name))
  , stoppable_callback_(std::move(stoppable_callback))
  , stop_requested_(false)
  , spin_period_(spin_period) {
}

Spinner::~Spinner() {
  if (async_spinner_handle_.valid()) {
    log(FATAL, "Spinner is still running. Call stop() before destroying the object.");
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

  const auto timestamp_start = std::chrono::system_clock::now();
  auto current_timestamp = std::chrono::system_clock::now();
  auto previous_timestamp = std::chrono::system_clock::time_point{};
  utils::timing::StopWatch stop_watch;

  while (!stop_requested_.load()) {
    try {
      current_timestamp = std::chrono::system_clock::now();
      const auto previous_spin_duration = stop_watch.stop<std::chrono::microseconds>();
      stop_watch.start();

      if (stoppable_callback_() == SpinResult::STOP) {
        break;
      }
      const auto callback_duration = stop_watch.elapsed<std::chrono::microseconds>();

      if (component_name_.has_value()) {
        if (previous_timestamp != std::chrono::system_clock::time_point{}) {
          telemetry::record(
              telemetry::Metric{ .component = component_name_.value(),
                                 .tag = "spinner_timings",
                                 .timestamp = previous_timestamp,
                                 .values = { { "spin_duration_microsec", previous_spin_duration.count() } } });
        }

        telemetry::record(
            telemetry::Metric{ .component = component_name_.value(),
                               .tag = "spinner_timings",
                               .timestamp = current_timestamp,
                               .values = { { "callback_duration_microsec", callback_duration.count() } } });
      }
      previous_timestamp = current_timestamp;

      if (spin_period_.has_value()) {  // Throttle spinner to a fixed period if spin_period_ is provided
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_until(lock, internal::computeNextSpinTimestamp(timestamp_start,
                                                                       std::chrono::system_clock::now(),
                                                                       spin_period_.value()));
      }
    } catch (std::exception& e) {
      log(ERROR, "Spinner caught an exception, terminating", "error", e.what());
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

  return start_timestamp +
         std::chrono::duration_cast<std::chrono::system_clock::duration>(spin_count * spin_period);
}
}  // namespace internal
}  // namespace heph::concurrency
