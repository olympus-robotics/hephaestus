//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <csignal>

#include <range/v3/range/conversion.hpp>

namespace heph::utils {

/// \brief Use this class to block until a signal is received.
/// Usage:
/// ```
/// int main() {
///  // Do something
///  SignalHandlerStop::wait();
/// }
/// ```
class SignalHandlerStop {
public:
  static void wait();

private:
  SignalHandlerStop() = default;
  [[nodiscard]] static auto instance() -> SignalHandlerStop&;

  static auto signalHandler(int /*unused*/) -> void;

private:
  std::atomic_flag stop_flag_ = ATOMIC_FLAG_INIT;
};  // namespace heph::utils

}  // namespace heph::utils
