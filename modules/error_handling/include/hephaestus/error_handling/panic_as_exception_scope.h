//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

namespace heph::error_handling {

/// @brief  Scope guard to enable panic as exceptions within the scope.
/// This is useful for unit tests where we want to catch panics as exceptions.
class PanicAsExceptionScope {
public:
  PanicAsExceptionScope();
  ~PanicAsExceptionScope();

  PanicAsExceptionScope(const PanicAsExceptionScope&) = delete;
  PanicAsExceptionScope(PanicAsExceptionScope&&) = delete;

  auto operator=(const PanicAsExceptionScope&) -> PanicAsExceptionScope& = delete;
  auto operator=(PanicAsExceptionScope&&) -> PanicAsExceptionScope& = delete;

  /// @brief  Check whether panics should be thrown as exceptions.
  [[nodiscard]] static auto isEnabled() -> bool;
};

}  // namespace heph::error_handling
