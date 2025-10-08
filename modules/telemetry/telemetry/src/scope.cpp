//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/scope.h"

#include <string>
#include <utility>
#include <vector>

namespace heph::telemetry {
namespace {
// thread_local ensures that each thread gets its own instance of module_stack.
// This is crucial for correctness in multi-threaded applications.
thread_local std::vector<Scope::Context>
    modules_stack;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

Scope::Scope(std::string robot_name, std::string module) {
  modules_stack.emplace_back(std::move(robot_name), std::move(module));
}

Scope::~Scope() {
  modules_stack.pop_back();
}

auto getCurrentContext() -> const Scope::Context* {
  return modules_stack.empty() ? nullptr : &modules_stack.back();
}

}  // namespace heph::telemetry
