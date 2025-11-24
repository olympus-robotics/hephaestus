//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/utils.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#ifndef __WIN32__
#include <linux/limits.h>
#endif

#include <sys/param.h>  // NOLINT(misc-include-cleaner)
#include <unistd.h>

namespace heph::utils {
auto getHostName() -> std::string {
#ifndef __WIN32__
  char host_name[MAXHOSTNAMELEN + 1] = { '\0' };  // NOLINT(cppcoreguidelines-avoid-c-arrays,
                                                  // misc-include-cleaner)
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  const auto ret = ::gethostname(host_name, sizeof(host_name));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  return ((ret == 0) ? (std::string(host_name)) : (""));
#else
#warning "getHostName is not implemented on Windows"
  return "";
#endif
}

auto getBinaryPath() -> std::optional<std::filesystem::path> {
#ifndef __WIN32__
  std::array<char, PATH_MAX> result{};
  auto count = static_cast<std::size_t>(readlink("/proc/self/exe", result.data(), PATH_MAX));
  if (count == 0) {
    return std::nullopt;
  }
  return std::filesystem::path{ std::string{ result.data(), count } };
#else
#warning "getBinaryPath is not implemented on Windows"
  return std::nullopt;
#endif
}
}  // namespace heph::utils
