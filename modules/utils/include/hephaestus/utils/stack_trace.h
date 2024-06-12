//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <memory>

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

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace heph::utils
