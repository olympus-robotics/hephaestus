//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <csignal>
#include <future>

#include "hephaestus/utils/concepts.h"

namespace heph::utils {

/// \brief Use this class to block until a signal is received.
/// > NOTE: can be extended to call a generic callback when a signal is received.
/// Usage:
/// ```
/// int main() {
///  // Do something
///  InterruptHandler::wait();
/// }
/// // Or
/// while (InterruptHandler::stopRequested()) {
/// // Do something
/// }
/// ```
class InterruptHandler {
public:
  /// Return false if a signal has been received, true otherwise.
  [[nodiscard]] static auto stopRequested() -> bool;
  /// Blocks until a signal has been received.
  static void wait();

private:
  InterruptHandler() = default;
  [[nodiscard]] static auto instance() -> InterruptHandler&;

  static auto signalHandler(int /*unused*/) -> void;

private:
  std::atomic_flag stop_flag_ = ATOMIC_FLAG_INIT;
};  // namespace heph::utils

template <typename T>
concept StoppableAndWaitable = requires { Stoppable<T>&& Waitable<T>; };

/// \brief Use this class to block until a signal is received or the application completes.
/// Usage:
/// ```
/// int main() {
///   auto app = MyApp{};
///   app.start().get();
///   InterruptHandlerOrAppComplete::wait(app);
/// }
/// ```
/// > NOTE: `MayApp` needs to satify the stoppable compoenent interface.
class InterruptHandlerOrAppComplete {
public:
  template <StoppableAndWaitable T>
  void wait(T& app);

private:
  InterruptHandlerOrAppComplete() = default;
  [[nodiscard]] static auto instance() -> InterruptHandlerOrAppComplete&;

  static auto signalHandler(int /*unused*/) -> void;

private:
  std::future<void> stop_future_;
  std::function<std::future<void>()> app_stop_cb_;
};

template <StoppableAndWaitable T>
void InterruptHandlerOrAppComplete::wait(T& app) {
  instance().app_stop_cb_ = [&app]() { return app.stop(); };

  (void)signal(SIGINT, InterruptHandlerOrAppComplete::signalHandler);
  (void)signal(SIGTERM, InterruptHandlerOrAppComplete::signalHandler);

  app.wait();

  if (instance().stop_future_.valid()) {
    instance().stop_future_.get();
  } else {
    app.stop().get();
  }
}

auto InterruptHandlerOrAppComplete::instance() -> InterruptHandlerOrAppComplete& {
  static InterruptHandlerOrAppComplete instance;
  return instance;
}

auto InterruptHandlerOrAppComplete::signalHandler(int /*unused*/) -> void {
  instance().stop_future_ = instance().app_stop_cb_();
}

}  // namespace heph::utils
