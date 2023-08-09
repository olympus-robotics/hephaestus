//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/types_protobuf/pose.h"

#include "eolo/serdes/protobuf/protobuf.h"
#include "eolo/types_protobuf/geometry.h"

namespace eolo::types {
void toProto(proto::Pose& proto_pose, const Pose& pose) {
  toProto(*proto_pose.mutable_position(), pose.position);
  toProto(*proto_pose.mutable_orientation(), pose.orientation);
}

void fromProto(const proto::Pose& proto_pose, Pose& pose) {
  fromProto(proto_pose.position(), pose.position);
  fromProto(proto_pose.orientation(), pose.orientation);
}

void toProtobuf(serdes::protobuf::SerializerBuffer& buffer, const Pose& pose) {
  serdes::protobuf::toProtobuf<proto::Pose>(buffer, pose);
}

void fromProtobuf(serdes::protobuf::DeserializerBuffer& buffer, Pose& pose) {
  serdes::protobuf::fromProtobuf<proto::Pose>(buffer, pose);
}
}  // namespace eolo::types
