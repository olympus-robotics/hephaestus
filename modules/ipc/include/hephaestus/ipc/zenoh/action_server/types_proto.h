//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/ipc/zenoh/action_server/proto/types.pb.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/enums.h"

namespace heph::serdes::protobuf {

template <typename RequestT>
struct ProtoAssociation<heph::ipc::zenoh::action_server::Request<RequestT>> {
  using Type = heph::ipc::zenoh::action_server::proto::Request;
};

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

template <typename RequestT>
void toProto(proto::Request& proto_request, const Request<RequestT>& request) {
  using ProtoT = heph::serdes::protobuf::ProtoAssociation<RequestT>::Type;
  ProtoT proto_value{};
  toProto(proto_value, request.request);
  proto_request.mutable_request()->PackFrom(proto_value);

  proto_request.set_response_service_topic_name(request.response_service_topic_name);
  proto_request.set_status_topic_name(request.status_topic_name);
}

template <typename RequestT>
void fromProto(const proto::Request& proto_request, Request<RequestT>& request) {
  using ProtoT = heph::serdes::protobuf::ProtoAssociation<RequestT>::Type;
  ProtoT proto_value{};
  proto_request.request().UnpackTo(&proto_value);
  fromProto(proto_value, request.request);

  request.response_service_topic_name = proto_request.response_service_topic_name();
  request.status_topic_name = proto_request.status_topic_name();
}

void toProto(proto::RequestResponse& proto_response, const RequestResponse& response);

void fromProto(const proto::RequestResponse& proto_response, RequestResponse& response);

template <typename ReplyT>
void toProto(proto::Response& proto_response, const Response<ReplyT>& response) {
  proto_response.set_status(serdes::protobuf::toProtoEnum<proto::RequestStatus>(response.status));

  using ProtoT = heph::serdes::protobuf::ProtoAssociation<ReplyT>::Type;
  ProtoT proto_value{};
  toProto(proto_value, response.value);
  proto_response.mutable_value()->PackFrom(proto_value);
}

template <typename ReplyT>
void fromProto(const proto::Response& proto_response, Response<ReplyT>& response) {
  serdes::protobuf::fromProto(proto_response.status(), response.status);

  using ProtoT = heph::serdes::protobuf::ProtoAssociation<ReplyT>::Type;
  ProtoT proto_value{};
  proto_response.value().UnpackTo(&proto_value);
  fromProto(proto_value, response.value);
}

}  // namespace heph::ipc::zenoh::action_server
