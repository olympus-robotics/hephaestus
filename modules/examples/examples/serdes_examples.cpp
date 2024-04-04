//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/serdes/serdes.h"

//=================================================================================================
auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;

  heph::examples::types::Pose pose;
  pose.position = Eigen::Vector3d{ 1, 2, 3 };
  pose.orientation =
      Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 };  // NOLINT(cppcoreguidelines-avoid-magic-numbers)

  const auto json_string = heph::serdes::serializeToJSON(pose);
  fmt::print("Pose serialized to JSON:\n{}\n", json_string);

  const auto txt_string = heph::serdes::serializeToText(pose);
  fmt::print("Pose serialized to text:\n{}\n", txt_string);
}
