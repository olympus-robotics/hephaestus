//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <csignal>

#include <range/v3/range/conversion.hpp>

namespace heph::utils {

/// \brief Use this class to block until a signal is received.
/// > NOTE: can be extended to call a generic callback when a signal is received.
/// Usage:
/// ```
/// int main() {
///  // Do something
///  SignalHandlerStop::wait();
/// }
/// // Or
/// while (SignalHandlerStop::ok()) {
/// // Do something
/// }
/// ```
class SignalHandlerStop {
public:
  /// Return false if a signal has been received, true otherwise.
  [[nodiscard]] static auto ok() -> bool;
  /// Blocks until a signal has been received.
  static void wait();

private:
  SignalHandlerStop() = default;
  [[nodiscard]] static auto instance() -> SignalHandlerStop&;

  static auto signalHandler(int /*unused*/) -> void;

private:
  std::atomic_flag stop_flag_ = ATOMIC_FLAG_INIT;
};  // namespace heph::utils

}  // namespace heph::utils
