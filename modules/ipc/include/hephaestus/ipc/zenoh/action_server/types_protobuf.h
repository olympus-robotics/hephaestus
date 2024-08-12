//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/ipc/zenoh/action_server/proto/types.pb.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::ipc::zenoh::action_server::RequestResponse> {
  using Type = heph::ipc::zenoh::action_server::proto::RequestResponse;
};

template <typename ReplyT>
struct ProtoAssociation<heph::ipc::zenoh::action_server::Response<ReplyT>> {
  using Type = heph::ipc::zenoh::action_server::proto::Response;
};
// Response

}  // namespace heph::serdes::protobuf

namespace heph::ipc::zenoh::action_server {

void toProto(proto::RequestStatus& proto_status, const RequestStatus& status);

void fromProto(const proto::RequestStatus& proto_status, RequestStatus& status);

void toProto(proto::RequestResponse& proto_response, const RequestResponse& response);

void fromProto(const proto::RequestResponse& proto_response, RequestResponse& response);

template <typename ReplyT>
void toProto(proto::Response& proto_response, const Response<ReplyT>& response) {
  proto::RequestStatus proto_status{};
  toProto(proto_status, response.status);
  proto_response.set_status(proto_status);

  using ProtoT = heph::serdes::protobuf::ProtoAssociation<ReplyT>::Type;
  ProtoT proto_value{};
  toProto(proto_value, response.value);
  proto_response.mutable_value()->PackFrom(proto_value);
}

template <typename ReplyT>
void fromProto(const proto::Response& proto_response, Response<ReplyT>& response) {
  fromProto(proto_response.status(), response.status);

  using ProtoT = heph::serdes::protobuf::ProtoAssociation<ReplyT>::Type;
  ProtoT proto_value{};
  proto_response.value().UnpackTo(&proto_value);
  fromProto(proto_value, response.value);
}

}  // namespace heph::ipc::zenoh::action_server
