//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <format>

#include <Eigen/Dense>

#include <fmt/format.h>

namespace heph::examples::types {

struct Pose {
  Eigen::Quaterniond orientation = Eigen::Quaterniond::Identity();
  Eigen::Vector3d position = Eigen::Vector3d::Zero();

  friend auto operator==(const Pose&, const Pose&) -> bool = default;
};

[[nodiscard]] inline auto toString(const Pose& pose) -> std::string {
  return std::format("(t: [{}, {}, {}], q[w,x,y,z]: [{}, {}, {}, {}])", pose.position.x(), pose.position.y(),
                     pose.position.z(), pose.orientation.w(), pose.orientation.x(), pose.orientation.y(),
                     pose.orientation.z());
}

}  // namespace heph::examples::types

template <>
struct std::formatter<heph::examples::types::Pose> final : public std::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const heph::examples::types::Pose& pose, FormatContext& ctx) {
    return std::format_to(ctx.out(), toString(pose));
  }
};

template <>
struct fmt::formatter<heph::examples::types::Pose> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const heph::examples::types::Pose& pose, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}", toString(pose));
  }
};
