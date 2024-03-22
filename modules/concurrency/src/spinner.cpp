//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include <absl/log/log.h>
#include <fmt/format.h>

#include "hephaestus/base/exception.h"

namespace heph::concurrency {
Spinner::Spinner() : is_started_(false) {
}

Spinner::~Spinner() {
  if (is_started_.load() || spinner_thread_.joinable()) {
    LOG(FATAL) << "Spinner is still running. Call stop() before destroying the object.";
    std::terminate();
  }
}

void Spinner::start() {
  throwExceptionIf<InvalidOperationException>(is_started_.load(), fmt::format("Spinner is already started."));

  spinner_thread_ = std::jthread([this](std::stop_token stop_token) { spin(stop_token); });

  is_started_.store(true);
}

void Spinner::spin(std::stop_token& stop_token) {
  while (!stop_token.stop_requested()) {
    spinOnce();
  }
}

void Spinner::stop() {
  throwExceptionIf<InvalidOperationException>(!is_started_.load(),
                                              fmt::format("Spinner not yet started, cannot stop."));
  spinner_thread_.request_stop();
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
