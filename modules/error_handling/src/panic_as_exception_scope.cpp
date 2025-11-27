//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/error_handling/panic_as_exception_scope.h"

#include <cstddef>

namespace heph::error_handling {

namespace {
// thread_local ensures that each thread gets its own instance of module_stack.
// This is crucial for correctness in multi-threaded applications.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local std::size_t panic_as_exception_counter = 0;
}  // namespace

PanicAsExceptionScope::PanicAsExceptionScope() {
  ++panic_as_exception_counter;
}

PanicAsExceptionScope::~PanicAsExceptionScope() {
  --panic_as_exception_counter;
}

auto PanicAsExceptionScope::isEnabled() -> bool {
  return panic_as_exception_counter > 0;
}

}  // namespace heph::error_handling
