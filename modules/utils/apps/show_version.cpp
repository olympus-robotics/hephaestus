//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include <cstdio>
#include <cstdlib>
#include <tuple>

#include <fmt/core.h>

#include "hephaestus/utils/version.h"

//=================================================================================================
auto main() -> int {
  try {
    const auto vn = heph::utils::getVersion();
    const auto bi = heph::utils::getBuildInfo();
    fmt::println("Version    : {:d}.{:d}.{:d}", vn.major, vn.minor, vn.patch);
    fmt::println("Build Info : '{}' branch, '{}' profile, '{}' hash", bi.branch, bi.profile, bi.hash);
    return EXIT_SUCCESS;
  } catch (...) {
    std::ignore = std::fputs("Exception occurred", stderr);
    return EXIT_FAILURE;
  }
}
