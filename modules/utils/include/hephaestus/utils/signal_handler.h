//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <csignal>
#include <future>

#include "hephaestus/utils/concepts.h"

namespace heph::utils {

template <typename T>
concept StoppableAndWaitable = requires { Stoppable<T>&& Waitable<T>; };

/// \brief Use this class to block until a signal is received.
/// > NOTE: can be extended to call a generic callback when a signal is received.
/// Usage:
/// ```
/// int main() {
///  // Do something
///  TerminationBlocker::waitForInterrupt();
/// }
/// // Or
/// while (TerminationBlocker::stopRequested()) {
/// // Do something
/// }
/// ```
class TerminationBlocker {
public:
  /// Return false if a signal has been received, true otherwise.
  [[nodiscard]] static auto stopRequested() -> bool;

  /// Blocks until a signal has been received.
  static void waitForInterrupt();

  /// Wait returns when a signal is received or the app completes.
  template <StoppableAndWaitable T>
  static void waitForInterruptOrAppCompletion(T& app);

private:
  TerminationBlocker() = default;
  [[nodiscard]] static auto instance() -> TerminationBlocker&;

  static auto signalHandler(int /*unused*/) -> void;

private:
  std::atomic_flag stop_flag_ = ATOMIC_FLAG_INIT;
  std::future<void> stop_future_;
  std::function<std::future<void>()> app_stop_callback_ = []() { return std::future<void>{}; };
};  // namespace heph::utils

template <StoppableAndWaitable T>
void TerminationBlocker::waitForInterruptOrAppCompletion(T& app) {
  instance().app_stop_callback_ = [&app]() { return app.stop(); };

  (void)signal(SIGINT, TerminationBlocker::signalHandler);
  (void)signal(SIGTERM, TerminationBlocker::signalHandler);

  app.wait();

  if (instance().stop_future_.valid()) {
    instance().stop_future_.get();
  } else {
    app.stop().get();
  }
}

}  // namespace heph::utils
