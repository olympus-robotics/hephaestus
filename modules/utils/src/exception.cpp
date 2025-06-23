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

Panic::Panic(Panic::MessageWithLocation&& message)
  : std::runtime_error("[" + std::string(utils::string::truncate(message.location.file_name(), "modules")) + ":" +
                       std::to_string(message.location.line()) + "] " + message.value + "\n" + utils::StackTrace::print()) {
}

}  // namespace heph
