//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/types_protobuf/pose.h"

#include "eolo/base/exception.h"
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
  proto::Pose proto_pose;
  toProto(proto_pose, pose);
  buffer.serialize(proto_pose);
}

void fromProtobuf(serdes::protobuf::DeserializerBuffer& buffer, Pose& pose) {
  proto::Pose proto_pose;
  auto res = buffer.deserialize(proto_pose);
  throwExceptionIf<InvalidDataException>(!res, "Failed to parse proto::pose from incoming buffer");

  fromProto(proto_pose, pose);
}
}  // namespace eolo::types
