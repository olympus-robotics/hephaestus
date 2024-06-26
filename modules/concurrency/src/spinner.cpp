//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include <chrono>
#include <future>

#include <absl/log/log.h>
#include <fmt/format.h>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency {
Spinner::Spinner(std::chrono::microseconds spin_period /*= std::chrono::microseconds{0}*/)
  : is_started_(false), stop_requested_(false), spin_period_(spin_period) {
}

Spinner::~Spinner() {
  if (is_started_.load() || spinner_thread_.joinable()) {
    LOG(FATAL) << "Spinner is still running. Call stop() before destroying the object.";
    std::terminate();
  }
}

void Spinner::start() {
  throwExceptionIf<InvalidOperationException>(is_started_.load(), fmt::format("Spinner is already started."));

  // NOTE: Replace with std::stop_token and std::jthread when clang supports it.
  spinner_thread_ = std::thread([this]() { spin(); });

  is_started_.store(true);
}

void Spinner::spin() {
  start_timestamp_ = std::chrono::system_clock::now();

  while (!stop_requested_.load()) {
    spinOnce();

    if (spin_period_.count() == 0) {
      continue;
    }

    const auto target_timestamp = start_timestamp_ + (++spin_count_) * spin_period_;
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_until(lock, target_timestamp);
  }
}

auto Spinner::stop() -> std::future<void> {
  throwExceptionIf<InvalidOperationException>(!is_started_.load(),
                                              fmt::format("Spinner not yet started, cannot stop."));
  stop_requested_.store(true);
  condition_.notify_all();

  return std::async(std::launch::async, [this]() { stopImpl(); });
}

void Spinner::stopImpl() {
  spinner_thread_.join();

  if (stop_callback_) {
    stop_callback_();
  }

  is_started_.store(false);
}

void Spinner::addStopCallback(std::function<void()> callback) {
  stop_callback_ = std::move(callback);
}

}  // namespace heph::concurrency
