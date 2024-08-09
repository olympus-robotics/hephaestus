//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/utils.h"

#include <string>

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
}  // namespace heph::utils
