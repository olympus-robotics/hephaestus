//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include <absl/log/log.h>
#include <fmt/format.h>

#include "hephaestus/base/exception.h"

namespace heph::concurrency {
Spinner::Spinner() : is_started_(false), stop_requested_(false) {
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
  while (!stop_requested_.load()) {
    spinOnce();
  }
}

void Spinner::stop() {
  throwExceptionIf<InvalidOperationException>(!is_started_.load(),
                                              fmt::format("Spinner not yet started, cannot stop."));
  stop_requested_.store(true);
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
