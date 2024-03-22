//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

#include "hephaestus/base/exception.h"

namespace heph::concurrency {
Spinner::Spinner() : is_started_(false), stop_token_(std::nullopt), spinner_thread_() {
}

virtual ~Spinner::Spinner() {
  stop();
}

void Spinner::start() {
  throwExceptionIf<InvalidOperationException>(is_started_.load(), fmt::format("Spinner is already started."));

  stop_token_.emplace();
  spinner_thread_ = std::jthread([this](std::stop_token st) { spin(); }, stop_token_.value());

  is_started_.store(true);
}

virtual void Spinner::spin() {
  while (!stop_token_->stop_requested()) {
    spinOnce();
  }
}

void Spinner::stop() {
  throwExceptionIf<InvalidOperationException>(!is_started_.load(),
                                              fmt::format("Spinner not yet started, cannot stop."));
  if (stop_token_) {
    stop_token_->request_stop();
    spinner_thread_.join();
    stop_token_.reset();
  }

  if (stop_callback_) {
    stop_callback_();
  }

  is_started_.store(false);
}

}  // namespace heph::concurrency
