//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/exception.h"

#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

#include "hephaestus/utils/stack_trace.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph {

Panic::Panic(std::string message, const std::source_location& location)
  : std::runtime_error("[" + std::string(utils::string::truncate(location.file_name(), "modules")) + ":" +
                       std::to_string(location.line()) + "] " + std::move(message) + "\n" +
                       utils::StackTrace::print()) {
}

}  // namespace heph
