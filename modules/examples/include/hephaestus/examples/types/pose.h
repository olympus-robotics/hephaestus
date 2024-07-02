//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <Eigen/Dense>

#include <fmt/format.h>

namespace heph::examples::types {

struct Pose {
  Eigen::Quaterniond orientation = Eigen::Quaterniond::Identity();
  Eigen::Vector3d position = Eigen::Vector3d::Zero();

  friend auto operator==(const Pose&, const Pose&) -> bool = default;
};

struct FramedPose {
  std::string frame;
  Pose pose;

  friend auto operator==(const FramedPose&, const FramedPose&) -> bool = default;
};

[[nodiscard]] inline auto toString(const Pose& pose) -> std::string {
  return fmt::format("(t: [{}, {}, {}], q[w,x,y,z]: [{}, {}, {}, {}])", pose.position.x(), pose.position.y(),
                     pose.position.z(), pose.orientation.w(), pose.orientation.x(), pose.orientation.y(),
                     pose.orientation.z());
}

[[nodiscard]] auto toString(const FramedPose& pose) -> std::string;

}  // namespace heph::examples::types

template <>
struct fmt::formatter<heph::examples::types::Pose> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const heph::examples::types::Pose& pose, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}", toString(pose));
  }
};

template <>
struct fmt::formatter<heph::examples::types::FramedPose> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const heph::examples::types::FramedPose& pose, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}", toString(pose));
  }
};
