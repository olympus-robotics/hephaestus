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
#include <thread>
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

Spinner::Spinner(Callback&& callback, double rate_hz /*= 0*/)
  : callback_(std::move(callback))
  , is_started_(false)
  , stop_requested_(false)
  , spin_period_(rateToPeriod(rate_hz)) {
}

Spinner::~Spinner() {
  if (is_started_.load()) {
    LOG(FATAL) << "Spinner is still running. Call stop() before destroying the object.";
    std::terminate();
  }

  if (async_spinner_handle_.valid()) {
    try {
      async_spinner_handle_.get();
    } catch (const std::exception& e) {
      throw e;  // Re-throw the exception to be handled by the caller.
    }
  }
}

void Spinner::start() {
  throwExceptionIf<InvalidOperationException>(is_started_.load(), "Spinner is already started.");

  std::promise<void> promise;
  async_spinner_handle_ = std::async(std::launch::async, [this, promise = std::move(promise)]() mutable {
    try {
      spin();
      promise.set_value();
    } catch (const std::exception& e) {
      promise.set_exception(std::current_exception());
    }
  });

  is_started_.store(true);
}

void Spinner::spin() {
  // TODO: set thread name

  start_timestamp_ = std::chrono::system_clock::now();

  while (!stop_requested_.load()) {
    const auto spin_result = callback_();

    ++spin_count_;

    if (spin_result == SpinResult::Stop) {
      break;
    }

    if (spin_period_.count() == 0) {
      continue;
    }

    const auto target_timestamp = start_timestamp_ + spin_count_ * spin_period_;
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_until(lock, target_timestamp);
  }
}

auto Spinner::stop() -> std::future<void> {
  throwExceptionIf<InvalidOperationException>(!is_started_.load(), "Spinner not yet started, cannot stop.");
  stop_requested_.store(true);
  condition_.notify_all();

  return std::async(std::launch::async, [this]() { stopImpl(); });
}

void Spinner::stopImpl() {
  is_started_.store(false);
}

auto Spinner::spinCount() const -> uint64_t {
  return spin_count_;
}

}  // namespace heph::concurrency
