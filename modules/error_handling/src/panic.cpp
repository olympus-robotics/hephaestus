//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/error_handling/panic.h"

#include <cstddef>
#include <exception>
#include <source_location>
#include <string>

#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::error_handling {
namespace {
// thread_local ensures that each thread gets its own instance of module_stack.
// This is crucial for correctness in multi-threaded applications.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local size_t panic_as_exception_counter = 0;
}  // namespace

PanicAsExceptionScope::PanicAsExceptionScope() {
  ++panic_as_exception_counter;
}

PanicAsExceptionScope::~PanicAsExceptionScope() {
  --panic_as_exception_counter;
}

auto panicAsException() -> bool {
  return panic_as_exception_counter > 0;
}

namespace detail {

void panicImpl(const std::source_location& location, const std::string& formatted_message) {
  auto location_str = std::string(utils::string::truncate(location.file_name(), "modules")) + ":" +
                      std::to_string(location.line());

  log(ERROR, "program terminated with panic", "error", formatted_message, "location", location_str);
  telemetry::flushLogEntries();

  if (error_handling::panicAsException()) {
    throw error_handling::PanicException(formatted_message);
  }

  std::terminate();
}

}  // namespace detail

}  // namespace heph::error_handling
