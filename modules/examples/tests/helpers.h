#pragma once

#include <random>

#include "hephaestus/examples/types/pose.h"

namespace heph::examples::types::tests {

inline auto randomString(std::mt19937_64& mt) -> std::string {
  static constexpr auto RANDOM_STRING_LENGTH = 10;
  static constexpr auto RANDOM_STRING_CHARS =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  std::string str;
  str.reserve(RANDOM_STRING_LENGTH);
  for (size_t i = 0; i < RANDOM_STRING_LENGTH; ++i) {
    str.push_back(RANDOM_STRING_CHARS[mt() % (sizeof(RANDOM_STRING_CHARS) -  // NOLINT
                                              1)]);
  }
  return str;
}

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
  pose.frame = randomString(mt);
  pose.pose = randomPose(mt);
  return pose;
}
}  // namespace heph::examples::types::tests
