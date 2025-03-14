//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/exception.h"

#include <source_location>
#include <stdexcept>
#include <string>

#include "hephaestus/utils/stack_trace.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph {

Panic::Panic(const std::string& message, std::source_location location)
  : std::runtime_error("[" + std::string(utils::string::truncate(location.file_name(), "modules")) + ":" +
                       std::to_string(location.line()) + "] " + message + "\n" + utils::StackTrace::print()) {
}

}  // namespace heph
