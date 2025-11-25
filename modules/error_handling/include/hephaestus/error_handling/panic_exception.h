//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <stdexcept>
#include <string>

namespace heph::error_handling {

class PanicException final : public std::runtime_error {
public:
  explicit PanicException(const std::string& message);
};

}  // namespace heph::error_handling
