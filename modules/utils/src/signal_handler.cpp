//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/signal_handler.h"

namespace heph::utils {

auto SignalHandlerStop::ok() -> bool {
  return instance().stop_flag_.test();
}

void SignalHandlerStop::wait() {
  (void)signal(SIGINT, SignalHandlerStop::signalHandler);
  (void)signal(SIGTERM, SignalHandlerStop::signalHandler);

  instance().stop_flag_.wait(false);
}

auto SignalHandlerStop::instance() -> SignalHandlerStop& {
  static SignalHandlerStop instance;
  return instance;
}

auto SignalHandlerStop::signalHandler(int /*unused*/) -> void {
  instance().stop_flag_.test_and_set();
  instance().stop_flag_.notify_all();
}
}  // namespace heph::utils
