#! /bin/sh

OUT=$(pwd)/$@

VERSION_TAG=$(git describe --tags --abbrev=0 || echo 'v0.0.0')
VERSION_MAJOR=$(echo $VERSION_TAG | cut -d'.' -f1 | tr -d 'v')
VERSION_MINOR=$(echo $VERSION_TAG | cut -d'.' -f2)
VERSION_PATCH=$(echo $VERSION_TAG | cut -d'.' -f3 | tr -cd '0-9')
REPO_BRANCH=$(git rev-parse --abbrev-ref HEAD)
REPO_HASH=$(git log -1 --format=%h)

cat << EOF > version_impl.h
//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================
#pragma once
#include <cstdint>
#include <string_view>

namespace heph::utils {
// These are autogenerated by the build system from version.in
static constexpr std::uint8_t VERSION_MAJOR = $VERSION_MAJOR;
static constexpr std::uint8_t VERSION_MINOR = $VERSION_MINOR;
static constexpr std::uint16_t VERSION_PATCH = $VERSION_PATCH;
static constexpr std::string_view REPO_BRANCH = "$REPO_BRANCH";
static constexpr std::string_view BUILD_PROFILE = "RelWithDebInfo";
static constexpr std::string_view REPO_HASH = "$REPO_HASH";
} // namespace heph::utils
EOF
cp version_impl.h $OUT
