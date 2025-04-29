//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

namespace heph::types {

/// @brief A class representing a UUID (Universally Unique Identifier).
struct UUID {
    [[nodiscard]] auto operator==(const UUID&) const -> bool = default;

    [[nodiscard]] auto format() const -> std::string;
    
    uint64_t high; //!< High 64 bits of the 128 bit UUID
    uint64_t low;  //!< Low 64 bits of the 128 bit UUID
};

}  // namespace heph::types
