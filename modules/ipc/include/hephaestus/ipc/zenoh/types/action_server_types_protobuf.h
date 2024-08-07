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

template <typename ReplyT>
struct ProtoAssociation<heph::ipc::zenoh::ActionServerResponse<ReplyT>> {
  using Type = heph::ipc::zenoh::proto::ActionServerResponse;
};
// ActionServerResponse

}  // namespace heph::serdes::protobuf

namespace heph::ipc::zenoh {

void toProto(proto::ActionServerRequestStatus& proto_status, const ActionServerRequestStatus& status);

void fromProto(const proto::ActionServerRequestStatus& proto_status, ActionServerRequestStatus& status);

void toProto(proto::ActionServerRequestResponse& proto_response, const ActionServerRequestResponse& response);

void fromProto(const proto::ActionServerRequestResponse& proto_response,
               ActionServerRequestResponse& response);

template <typename ReplyT>
void toProto(proto::ActionServerResponse& proto_response, const ActionServerResponse<ReplyT>& response) {
  proto::ActionServerRequestStatus proto_status{};
  toProto(proto_status, response.status);
  proto_response.set_status(proto_status);

  using ProtoT = heph::serdes::protobuf::ProtoAssociation<ReplyT>::Type;
  ProtoT proto_value{};
  toProto(proto_value, response.value);
  proto_response.mutable_value()->PackFrom(proto_value);
}

template <typename ReplyT>
void fromProto(const proto::ActionServerResponse& proto_response, ActionServerResponse<ReplyT>& response) {
  fromProto(proto_response.status(), response.status);

  using ProtoT = heph::serdes::protobuf::ProtoAssociation<ReplyT>::Type;
  ProtoT proto_value{};
  proto_response.value().UnpackTo(&proto_value);
  fromProto(proto_value, response.value);
}

}  // namespace heph::ipc::zenoh
