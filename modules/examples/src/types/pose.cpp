//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/examples/types/pose.h"  // NOLINT(misc-include-cleaner)

#include <string>

#include <fmt/format.h>

namespace heph::examples::types {
[[nodiscard]] inline auto toString(const FramedPose& pose) -> std::string {
  return fmt::format("(frame: {}, pose: {}", pose.frame, pose.pose);
}
}  // namespace heph::examples::types
