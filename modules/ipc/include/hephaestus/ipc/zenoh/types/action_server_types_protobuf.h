//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/ipc/zenoh/proto/action_server_types.pb.h"
#include "hephaestus/ipc/zenoh/types/action_server_types.h"
#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::ipc::zenoh::ActionServerRequestResponse> {
  using Type = heph::ipc::zenoh::proto::ActionServerRequestResponse;
};

}  // namespace heph::serdes::protobuf

namespace heph::ipc::zenoh {

void toProto(proto::ActionServerRequestResponse& proto_response, const ActionServerRequestResponse& response);

void fromProto(const proto::ActionServerRequestResponse& proto_response,
               ActionServerRequestResponse& response);

}  // namespace heph::ipc::zenoh
