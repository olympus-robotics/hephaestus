//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/utils/version.h"

#include "version_impl.h"

namespace heph::utils {
auto getBuildInfo() -> BuildInfo {
  return { .branch = REPO_BRANCH, .profile = BUILD_PROFILE, .hash = REPO_HASH };
}

auto getVersion() -> Version {
  return { .major = VERSION_MAJOR, .minor = VERSION_MINOR, .patch = VERSION_PATCH };
}

}  // namespace heph::utils
