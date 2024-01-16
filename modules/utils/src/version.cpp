//=================================================================================================
// Copyright (C) 2018-2023 EOLO Contributors
// MIT License
//=================================================================================================

#include "eolo/utils/version.h"

#include "version_impl.h"

namespace eolo::utils {
auto getBuildInfo() -> BuildInfo {
  return { REPO_BRANCH, BUILD_PROFILE, REPO_HASH };
}

auto getVersion() -> Version {
  return { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH };
}

}  // namespace eolo::utils
