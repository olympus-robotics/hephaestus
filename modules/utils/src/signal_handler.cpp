//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/signal_handler.h"

#include <csignal>
#include <functional>
#include <mutex>
#include <utility>

namespace heph::utils {

auto TerminationBlocker::stopRequested() -> bool {
  static std::once_flag flag;
  std::call_once(flag, []() {
    (void)signal(SIGINT, TerminationBlocker::signalHandler);
    (void)signal(SIGTERM, TerminationBlocker::signalHandler);
  });

  return instance().stop_flag_.test();
}

void TerminationBlocker::waitForInterrupt() {
  (void)signal(SIGINT, TerminationBlocker::signalHandler);
  (void)signal(SIGTERM, TerminationBlocker::signalHandler);

  instance().stop_flag_.wait(false);
}

void TerminationBlocker::registerInterruptCallback(std::function<void()>&& interrupt_callback) {
  (void)signal(SIGINT, TerminationBlocker::signalHandler);
  (void)signal(SIGTERM, TerminationBlocker::signalHandler);

  instance().interrupt_callback_ = std::move(interrupt_callback);
}

auto TerminationBlocker::instance() -> TerminationBlocker& {
  static TerminationBlocker instance;
  return instance;
}

void TerminationBlocker::signalHandler(int /*unused*/) {
  instance().stop_future_ = instance().app_stop_callback_();
  instance().interrupt_callback_();

  instance().stop_flag_.test_and_set();
  instance().stop_flag_.notify_all();
}
}  // namespace heph::utils
