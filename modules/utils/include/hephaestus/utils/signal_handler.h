//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <csignal>

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

}  // namespace heph::utils
