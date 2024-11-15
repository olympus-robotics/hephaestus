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
[[nodiscard]] auto rateToPeriod(std::optional<double> rate_hz) -> std::optional<std::chrono::microseconds> {
  if (!rate_hz.has_value()) {
    return std::nullopt;
  }

  const double period_seconds = 1. / rate_hz.value();
  const auto period_microseconds = static_cast<uint64_t>(period_seconds * 1e6);

  return std::chrono::microseconds{ period_microseconds };
}
}  // namespace

Spinner::Spinner(SpinnerStateMachineCallback&& state_machine_callback, std::optional<double> rate_hz /*= std::nullopt*/)
  : state_machine_callback_(std::move(state_machine_callback)), stop_requested_(false), spin_period_(rateToPeriod(rate_hz)) {
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

  while (!stop_requested_.load()) {
    state = state_machine_callback_(state);
    if (state_ == State::EXIT) {
      break;
    }

    // Logic to handle periodic spinning at a fixed rate. Initialization, execution etc. are handled at the same rate.
    if (!spin_period_.has_value()) {
      continue;
    }
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
