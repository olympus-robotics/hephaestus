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
}  // namespace heph::serdes::protobuf

namespace heph::examples::types {
void toProto(proto::Pose& proto_pose, const Pose& pose);
void fromProto(const proto::Pose& proto_pose, Pose& pose);
}  // namespace heph::examples::types
