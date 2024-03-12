//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <cstdint>
#include <string_view>

namespace heph::utils
{

// These are autogenerated by the build system from version.in

static constexpr std::uint8_t VERSION_MAJOR = 0;
static constexpr std::uint8_t VERSION_MINOR = 0;
static constexpr std::uint16_t VERSION_PATCH = 1;

static constexpr std::string_view REPO_BRANCH = "fix_format_submodule";
static constexpr std::string_view BUILD_PROFILE = "RelWithDebInfo";
static constexpr std::string_view REPO_HASH = "d5dd1f4";

} // namespace heph::utils
