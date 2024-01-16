//=================================================================================================
// Copyright (C) 2018-2023 EOLO Contributors
// MIT License
//=================================================================================================

#include <print>

#include "eolo/utils/version.h"

//=================================================================================================
auto main() -> int {
  try {
    const auto vn = eolo::utils::getVersion();
    const auto bi = eolo::utils::getBuildInfo();
    std::println("Version    : {:d}.{:d}.{:d}", vn.major, vn.minor, vn.patch);
    std::println("Build Info : '{}' branch, '{}' profile, '{}' hash", bi.branch, bi.profile, bi.hash);
    return EXIT_SUCCESS;
  } catch (...) {
    std::ignore = std::fputs("Exception occurred", stderr);
    return EXIT_FAILURE;
  }
}
