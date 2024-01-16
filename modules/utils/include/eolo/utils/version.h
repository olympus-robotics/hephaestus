//=================================================================================================
// Copyright (C) 2018-2023 EOLO Contributors
// MIT License
//=================================================================================================

#pragma once

#include <compare>
#include <cstdint>
#include <string_view>

namespace eolo::utils {

/// Software version data
struct [[nodiscard]] Version {
  std::uint8_t major{};
  std::uint8_t minor{};
  std::uint16_t patch{};
  constexpr auto operator<=>(const Version&) const = default;
};

/// Source configuration data
struct [[nodiscard]] BuildInfo {
  std::string_view branch;
  std::string_view profile;
  std::string_view hash;
};

/// @return Software version of project binaries
auto getVersion() -> Version;

/// @return Source configuration that generated project binaries
auto getBuildInfo() -> BuildInfo;

}  // namespace eolo::utils
