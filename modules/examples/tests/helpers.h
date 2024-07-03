#pragma once

#include <random>

#include "hephaestus/examples/types/pose.h"
#include <hephaestus/random/random_container.h>

namespace heph::examples::types::tests {

inline auto randomPose(std::mt19937_64& mt) -> Pose {
  static constexpr auto RANDOM_TRANSLATION_RANGE = 100.0;

  using DistT = std::uniform_real_distribution<double>;
  DistT t_distribution(RANDOM_TRANSLATION_RANGE);
  DistT r_distribution(-M_PI, M_PI);

  return { .orientation = Eigen::Quaterniond{ r_distribution(mt), r_distribution(mt), r_distribution(mt),
                                              r_distribution(mt) }
                              .normalized(),
           .position = Eigen::Vector3d{ t_distribution(mt), t_distribution(mt), t_distribution(mt) } };
}

inline auto randomFramedPose(std::mt19937_64& mt) -> FramedPose {
  FramedPose pose;
  pose.frame = random::randomT<std::string>(mt, std::nullopt, false);
  pose.pose = randomPose(mt);
  return pose;
}
}  // namespace heph::examples::types::tests
