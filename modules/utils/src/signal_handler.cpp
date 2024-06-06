//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/signal_handler.h"

namespace heph::utils {

auto InterruptHandler::stopRequested() -> bool {
  return instance().stop_flag_.test();
}

void InterruptHandler::wait() {
  (void)signal(SIGINT, InterruptHandler::signalHandler);
  (void)signal(SIGTERM, InterruptHandler::signalHandler);

  instance().stop_flag_.wait(false);
}

auto InterruptHandler::instance() -> InterruptHandler& {
  static InterruptHandler instance;
  return instance;
}

auto InterruptHandler::signalHandler(int /*unused*/) -> void {
  instance().stop_flag_.test_and_set();
  instance().stop_flag_.notify_all();
}
}  // namespace heph::utils
