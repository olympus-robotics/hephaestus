//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once
#include <format>

#include <Eigen/Dense>

#include <fmt/format.h>

namespace eolo::examples::types {

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

}  // namespace eolo::examples::types

template <>
struct std::formatter<eolo::examples::types::Pose> final : public std::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const eolo::examples::types::Pose& pose, FormatContext& ctx) {
    return std::format_to(ctx.out(), toString(pose));
  }
};

template <>
struct fmt::formatter<eolo::examples::types::Pose> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const eolo::examples::types::Pose& pose, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}", toString(pose));
  }
};
