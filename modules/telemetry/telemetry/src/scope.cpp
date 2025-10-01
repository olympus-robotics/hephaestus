//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/scope.h"

#include <string>
#include <vector>

namespace heph::telemetry {
namespace {
// thread_local ensures that each thread gets its own instance of module_stack.
// This is crucial for correctness in multi-threaded applications.
thread_local std::vector<std::string>
    modules_stack;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

Scope::Scope(std::string module) {
  modules_stack.push_back(std::move(module));
}

Scope::~Scope() {
  modules_stack.pop_back();
}

auto getModulesStack() -> const std::vector<std::string>& {
  return modules_stack;
}

}  // namespace heph::telemetry
