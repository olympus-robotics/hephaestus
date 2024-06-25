//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/examples/types_protobuf/pose.h"

#include <Eigen/Core>

#include <hephaestus/examples/types/proto/pose.pb.h>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/geometry.h"

namespace heph::examples::types {
void toProto(proto::Pose& proto_pose, const Pose& pose) {
  toProto(*proto_pose.mutable_position(), pose.position);
  toProto(*proto_pose.mutable_orientation(), pose.orientation);
}

void fromProto(const proto::Pose& proto_pose, Pose& pose) {
  fromProto(proto_pose.position(), pose.position);
  fromProto(proto_pose.orientation(), pose.orientation);
}

}  // namespace heph::examples::types
