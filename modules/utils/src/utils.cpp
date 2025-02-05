//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/utils.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include <linux/limits.h>
#include <sys/param.h>  // NOLINT(misc-include-cleaner)
#include <unistd.h>

namespace heph::utils {
auto getHostName() -> std::string {
  char host_name[MAXHOSTNAMELEN + 1] = { '\0' };  // NOLINT(cppcoreguidelines-avoid-c-arrays,
                                                  // misc-include-cleaner)
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  const auto ret = ::gethostname(host_name, sizeof(host_name));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  return ((ret == 0) ? (std::string(host_name)) : (""));
}

auto getBinaryPath() -> std::optional<std::filesystem::path> {
  std::array<char, PATH_MAX> result{};
  auto count = static_cast<std::size_t>(readlink("/proc/self/exe", result.data(), PATH_MAX));
  if (count == 0) {
    return std::nullopt;
  }
  return std::filesystem::path{ std::string{ result.data(), count } };
}
}  // namespace heph::utils
