//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
// MIT License
//=================================================================================================

#include "eolo/base/exception.h"

#include "eolo/utils/utils.h"

namespace eolo {

Exception::Exception(const std::string& message, std::source_location location)
  : std::runtime_error("[" + std::string(utils::truncate(location.file_name(), "modules")) + ":" +
                       std::to_string(location.line()) + "] " + message) {
}

}  // namespace eolo
