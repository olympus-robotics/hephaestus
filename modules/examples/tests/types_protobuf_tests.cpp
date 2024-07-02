//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <random>

#include <gtest/gtest.h>

#include "helpers.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types/proto/geometry.pb.h"
#include "hephaestus/examples/types/proto/pose.pb.h"
#include "hephaestus/examples/types_protobuf/geometry.h"
#include "hephaestus/examples/types_protobuf/pose.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::examples::types::tests {
// NOLINTBEGIN(misc-include-cleaner) // Unfortunately Eigen doesn't work well with this check
TEST(Geometry, StaticMatrix) {
  const Eigen::Matrix4d matrix4d = Eigen::Matrix4d::Random();
  proto::MatrixXd proto_matrix;
  toProto(proto_matrix, matrix4d);

  Eigen::Matrix4d matrix4d_des;
  fromProto(proto_matrix, matrix4d_des);

  EXPECT_EQ(matrix4d, matrix4d_des);
}

TEST(Geometry, DynamicMatrix1d) {
  static constexpr Eigen::Index MAX_SIZE = 1000;
  std::mt19937_64 mt{ std::random_device{}() };
  std::uniform_int_distribution<Eigen::Index> size_dist{ 1, MAX_SIZE };
  const auto size = size_dist(mt);

  const Eigen::Matrix<float, 2, -1> matrixf = Eigen::Matrix<float, 2, -1>::Random(2, size);
  EXPECT_EQ(matrixf.cols(), size);

  proto::MatrixXf proto_matrix;
  toProto(proto_matrix, matrixf);

  Eigen::Matrix<float, 2, -1> matrixf_des;
  fromProto(proto_matrix, matrixf_des);
  EXPECT_EQ(matrixf, matrixf_des);
}

TEST(Geometry, DynamicMatrix2d) {
  static constexpr Eigen::Index MAX_SIZE = 1000;
  std::mt19937_64 mt{ std::random_device{}() };
  std::uniform_int_distribution<Eigen::Index> size_dist{ 1, MAX_SIZE };
  const auto rows = size_dist(mt);
  const auto cols = size_dist(mt);

  const Eigen::Matrix<float, -1, -1> matrixf = Eigen::Matrix<float, -1, -1>::Random(rows, cols);

  proto::MatrixXf proto_matrix;
  toProto(proto_matrix, matrixf);

  Eigen::Matrix<float, -1, -1> matrixf_des;
  fromProto(proto_matrix, matrixf_des);
  EXPECT_EQ(matrixf, matrixf_des);
}

TEST(Geometry, DynamicVector) {
  static constexpr Eigen::Index MAX_SIZE = 1000;
  std::mt19937_64 mt{ std::random_device{}() };
  std::uniform_int_distribution<Eigen::Index> size_dist{ 1, MAX_SIZE };
  const auto size = size_dist(mt);

  const Eigen::VectorXf vector = Eigen::VectorX<float>::Random(size);

  proto::VectorXf proto_vector;
  toProto(proto_vector, vector);

  Eigen::VectorXf vector_des;
  fromProto(proto_vector, vector_des);
  EXPECT_EQ(vector, vector_des);
}

TEST(Geometry, StaticVector2) {
  const Eigen::Vector2f vector = Eigen::Vector2f::Random();
  proto::Vector2f proto_vector;
  toProto(proto_vector, vector);

  Eigen::Vector2f vector_des;
  fromProto(proto_vector, vector_des);
  EXPECT_EQ(vector, vector_des);
}

TEST(Geometry, StaticVector3) {
  const Eigen::Vector3d vector = Eigen::Vector3d::Random();
  proto::Vector3d proto_vector;
  toProto(proto_vector, vector);

  Eigen::Vector3d vector_des;
  fromProto(proto_vector, vector_des);
  EXPECT_EQ(vector, vector_des);
}

TEST(Pose, Pose) {
  std::mt19937_64 mt{ std::random_device{}() };
  const auto pose = randomPose(mt);

  proto::Pose proto_pose;
  toProto(proto_pose, pose);

  Pose pose_des;
  fromProto(proto_pose, pose_des);
  EXPECT_EQ(pose, pose_des);
}

TEST(FramedPose, FramedPose) {
  std::mt19937_64 mt{ std::random_device{}() };
  const auto pose = randomFramedPose(mt);

  proto::FramedPose proto_pose;
  toProto(proto_pose, pose);

  FramedPose pose_des;
  fromProto(proto_pose, pose_des);
  EXPECT_EQ(pose, pose_des);
}
// NOLINTEND(misc-include-cleaner)
}  // namespace heph::examples::types::tests
