//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "hephaestus/concurrency/spinner_state_machine.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry/metric_sink.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/timing/stop_watch.h"

namespace heph::concurrency {
namespace {
class SpinnerTelemetry {
public:
  explicit SpinnerTelemetry(std::optional<std::string> component_name)
    : component_name_(std::move(component_name)) {
  }

  /// Start and time previous spin
  void registerStartSpin() {
    current_timestamp_ = std::chrono::system_clock::now();
    previous_spin_duration_ = stop_watch_.stop<std::chrono::microseconds>();
    stop_watch_.start();
  }

  void timeCallback() {
    current_callback_duration_ = stop_watch_.elapsed<std::chrono::microseconds>();
  }

  // Record
  void recordMatrics() {
    if (!component_name_.has_value()) {
      return;
    }

    telemetry::record(telemetry::Metric{
        .component = component_name_.value(),
        .tag = "spinner_timings",
        .timestamp = current_timestamp_,
        .values = { { "callback_duration_microsec", current_callback_duration_.count() } } });

    // On the first spin, there is no previous spin_duration nor previous timestamp.
    if (previous_timestamp_ != std::chrono::system_clock::time_point{}) {
      telemetry::record(
          telemetry::Metric{ .component = component_name_.value(),
                             .tag = "spinner_timings",
                             .timestamp = previous_timestamp_,
                             .values = { { "spin_duration_microsec", previous_spin_duration_.count() } } });
    }
    previous_timestamp_ = current_timestamp_;
  }

private:
  std::optional<std::string> component_name_;

  std::chrono::system_clock::time_point current_timestamp_;
  std::chrono::system_clock::time_point previous_timestamp_;
  utils::timing::StopWatch stop_watch_;

  std::chrono::microseconds previous_spin_duration_{ 0 };
  std::chrono::microseconds current_callback_duration_{ 0 };
};
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
  SpinnerTelemetry spinner_telemetry(component_name_);

  while (!stop_requested_.load()) {
    try {
      spinner_telemetry.registerStartSpin();
      if (stoppable_callback_() == SpinResult::STOP) {
        break;
      }
      spinner_telemetry.timeCallback();
      spinner_telemetry.recordMatrics();

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
