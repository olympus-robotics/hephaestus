//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/error_handling/panic.h"

#include <exception>
#include <source_location>
#include <string>

#include "hephaestus/error_handling/panic_as_exception_scope.h"
#include "hephaestus/error_handling/panic_exception.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::error_handling::detail {

void panicImpl(const std::source_location& location, const std::string& formatted_message) {
  auto location_str = std::string(utils::string::truncate(location.file_name(), "modules")) + ":" +
                      std::to_string(location.line());

  log(ERROR, "program terminated with panic", "error", formatted_message, "location", location_str);
  telemetry::flushLogEntries();

  if (panicAsException()) {
    throw PanicException(formatted_message);
  }

  std::terminate();
}

}  // namespace heph::error_handling::detail
