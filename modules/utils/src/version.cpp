//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/utils/version.h"

#include "version_impl.h"

namespace heph::utils {
auto getBuildInfo() -> BuildInfo {
  return { REPO_BRANCH, BUILD_PROFILE, REPO_HASH };
}

auto getVersion() -> Version {
  return { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH };
}

}  // namespace heph::utils
