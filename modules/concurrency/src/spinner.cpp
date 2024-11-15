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

  const double period_seconds = 1. / rate_hz;
  const auto period_microseconds = static_cast<uint64_t>(period_seconds * 1e6);

  return std::chrono::microseconds{ period_microseconds };
}
}  // namespace

Spinner::Spinner(SpinnerCallbacks&& callbacks, double rate_hz /*= 0*/)
  : callbacks_(std::move(callbacks))
  , stop_requested_(false)
  , spin_period_(rateToPeriod(rate_hz)) {
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

  try {
    start_timestamp_ = std::chrono::system_clock::now();
    uint64_t spin_count_{ 0 };

    while (!stop_requested_.load()) {
      // State machine to handle the spinner lifecycle.
      if(state_ == State::NOT_INITIALIZED) {
        const auto error_message = init();
        state_ = error_message.has_value() ? State::INIT_FAILED : State::READY_TO_SPIN;
        LOG_IF(ERROR, error_message.has_value()) << fmt::format("Spinner initialization failed with message: {}", error_message.value());
      }

      if(state_ == State::READY_TO_SPIN) {
        const auto error_message = spinOnce();
        state_ = error_message.has_value() ? State::SPIN_FAILED : State::SPIN_SUCCESSFUL;
        LOG_IF(ERROR, error_message.has_value()) << fmt::format("Spinner spin failed with message: {}", error_message.value());
      }

      if(state_ == State::INIT_FAILED || state_ == State::SPIN_FAILED) {
        state_ = shall_re_init_() ? State::NOT_INITIALIZED : State::TERMINATE;
      }

      if(state_ == State::SPIN_SUCCESSFUL) {
        state_ = shall_stop_() ? State::TERMINATE : State::READY_TO_SPIN;
      }

      if(state_ == State::TERMINATE) {
        break;
      }

      // Logic to handle periodic spinning at a fixed rate.
      if (spin_period_.count() == 0) {
        continue;
      }
      const auto target_timestamp = start_timestamp_ + (++spin_count_ * spin_period_);
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait_until(lock, target_timestamp);
    }
  } catch (std::exception& e) {
    terminate();
    throw;  // TODO(@filippo) consider if we want to handle this error in a different way.
  }

  terminate();
}

auto Spinner::terminate() -> void {
  state_ = State::TERMINATING;
  spinner_completed_.test_and_set();
  spinner_completed_.notify_all();
  termination_callback_();
  state_ = State::TERMINATED;
}

auto Spinner::init() -> std::optional<ErrorMessageT>{
  try {
    state_ = State::INITIALIZING;
    init_callback_();
    return std::nullopt;
  } catch (std::exception& e) {
    return e.what();
  }
}

auto Spinner::spinOnce() -> std::optional<ErrorMessageT>{
  try {
    state_ = State::SPINNING;
    spin_once_callback_();
    return std::nullopt;
  } catch (std::exception& e) {
    return e.what();
  }
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

}  // namespace heph::concurrency
