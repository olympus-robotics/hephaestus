//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/error_handling/panic.h"

namespace heph::error_handling {
namespace {
// thread_local ensures that each thread gets its own instance of module_stack.
// This is crucial for correctness in multi-threaded applications.
thread_local bool panic_as_exception = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

PanicAsExceptionScope::PanicAsExceptionScope() {
  panic_as_exception = true;
}

PanicAsExceptionScope::~PanicAsExceptionScope() {
  panic_as_exception = false;
}

auto panicAsException() -> bool {
  return panic_as_exception;
}

}  // namespace heph::error_handling
