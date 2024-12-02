//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <exception>

#include <Eigen/Dense>

#include <fmt/base.h>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_proto/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/serdes/json.h"
#include "hephaestus/serdes/serdes.h"

//=================================================================================================
auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;
  try {
    heph::examples::types::Pose pose;
    pose.position = Eigen::Vector3d{ 1, 2, 3 };                  // NOLINT
    pose.orientation = Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 };  // NOLINT

    const auto json_string = heph::serdes::serializeToJSON(pose);
    fmt::print("Pose serialized to JSON:\n{}\n", json_string);

    const auto txt_string = heph::serdes::serializeToText(pose);
    fmt::print("Pose serialized to text:\n{}\n", txt_string);
  } catch (const std::exception& e) {
    fmt::print("Error: {}\n", e.what());
    return 1;
  }
}
