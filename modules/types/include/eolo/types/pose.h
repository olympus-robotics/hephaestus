//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <Eigen/Dense>

namespace eolo::types {

struct Pose {
  Eigen::Quaterniond orientation = Eigen::Quaterniond::Identity();
  Eigen::Vector3d position = Eigen::Vector3d::Zero();

  friend auto operator==(const Pose&, const Pose&) -> bool = default;
};

}  // namespace eolo::types
