//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/signal_handler.h"

#include <csignal>

namespace heph::utils {

auto TerminationBlocker::stopRequested() -> bool {
  return instance().stop_flag_.test();
}

void TerminationBlocker::waitForInterrupt() {
  (void)signal(SIGINT, TerminationBlocker::signalHandler);
  (void)signal(SIGTERM, TerminationBlocker::signalHandler);

  instance().stop_flag_.wait(false);
}

auto TerminationBlocker::instance() -> TerminationBlocker& {
  static TerminationBlocker instance;
  return instance;
}

auto TerminationBlocker::signalHandler(int /*unused*/) -> void {
  instance().stop_future_ = instance().app_stop_callback_();

  instance().stop_flag_.test_and_set();
  instance().stop_flag_.notify_all();
}
}  // namespace heph::utils
