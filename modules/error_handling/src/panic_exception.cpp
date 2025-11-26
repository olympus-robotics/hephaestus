//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/error_handling/panic_exception.h"

#include <stdexcept>
#include <string>

namespace heph::error_handling {

PanicException::PanicException(const std::string& message) : std::runtime_error(message) {
}

}  // namespace heph::error_handling
