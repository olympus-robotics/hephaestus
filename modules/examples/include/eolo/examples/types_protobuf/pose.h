//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include "eolo/examples/types/pose.h"
#include "eolo/examples/types/proto/pose.pb.h"
#include "eolo/serdes/protobuf/concepts.h"

namespace eolo::serdes::protobuf {
template <>
struct ProtoAssociation<eolo::examples::types::Pose> {
  using Type = eolo::examples::types::proto::Pose;
};
}  // namespace eolo::serdes::protobuf

namespace eolo::examples::types {
void toProto(proto::Pose& proto_pose, const Pose& pose);
void fromProto(const proto::Pose& proto_pose, Pose& pose);
}  // namespace eolo::examples::types
