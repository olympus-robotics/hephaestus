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
}  // namespace

Spinner::Spinner(StoppableCallback&& stoppable_callback, double rate_hz /*= 0*/)
  : stoppable_callback_(std::move(stoppable_callback))
  , stop_requested_(false)
  , spin_period_(rateToPeriod(rate_hz)) {
}

Spinner::Spinner(Callback&& callback, double rate_hz /*= 0*/)
  : Spinner(StoppableCallback([cb = std::move(callback)]() -> SpinResult {
              cb();
              return SpinResult::CONTINUE;
            }),
            rate_hz) {
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

    while (!stop_requested_.load()) {
      const auto spin_result = stoppable_callback_();

      ++spin_count_;

      if (spin_result == SpinResult::STOP) {
        break;
      }

      if (spin_period_.count() == 0) {
        continue;
      }

      const auto target_timestamp = start_timestamp_ + spin_count_ * spin_period_;
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait_until(lock, target_timestamp);
    }
  } catch (std::exception& e) {
    spinner_completed_.test_and_set();
    spinner_completed_.notify_all();
    termination_callback_();
    throw;  // TODO(@filippo) consider if we want to handle this error in a different way.
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

auto Spinner::spinCount() const -> uint64_t {
  return spin_count_;
}

void Spinner::setTerminationCallback(Callback&& termination_callback) {
  termination_callback_ = std::move(termination_callback);
}

}  // namespace heph::concurrency
