//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types/proto/pose.pb.h"
#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::examples::types::Pose> {
  using Type = heph::examples::types::proto::Pose;
};

template <>
struct ProtoAssociation<heph::examples::types::FramedPose> {
  using Type = heph::examples::types::proto::FramedPose;
};
}  // namespace heph::serdes::protobuf

namespace heph::examples::types {
void toProto(proto::Pose& proto_pose, const Pose& pose);
void fromProto(const proto::Pose& proto_pose, Pose& pose);

void toProto(proto::FramedPose& proto_pose, const FramedPose& pose);
void fromProto(const proto::FramedPose& proto_pose, FramedPose& pose);
}  // namespace heph::examples::types

#include "hephaestus/serdes/protobuf/protobuf.inl"
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::examples::types::Pose);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::examples::types::FramedPose);
