//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <memory>
#include <string>

namespace heph::utils {

/// \brief Class to print a stack trace on crash.
/// Usage:
/// int main() {
///   heph::utils::StackTrace stack_trace;
///   ...
/// }
class StackTrace {
public:
  StackTrace();
  ~StackTrace();

  static auto print() -> std::string;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace heph::utils
