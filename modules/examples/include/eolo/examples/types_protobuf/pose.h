//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include "eolo/examples/types/pose.h"
#include "eolo/examples/types/proto/pose.pb.h"
#include "eolo/serdes/protobuf/buffers.h"

namespace eolo::examples::types {
void toProto(proto::Pose& proto_pose, const Pose& pose);
void fromProto(const proto::Pose& proto_pose, Pose& pose);

void toProtobuf(serdes::protobuf::SerializerBuffer& buffer, const Pose& pose);
void fromProtobuf(serdes::protobuf::DeserializerBuffer& buffer, Pose& pose);

}  // namespace eolo::examples::types
