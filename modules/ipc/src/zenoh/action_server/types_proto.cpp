//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <absl/log/log.h>
#include <magic_enum.hpp>

#include "hephaestus/ipc/zenoh/action_server/proto/types.pb.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"

namespace heph::ipc::zenoh::action_server {
// TODO(@filippobrizzi): for some reason clang-tidy doesn't understand that this are public function declared
// in the header. NOLINTBEGIN(misc-use-internal-linkage)
void toProto(proto::RequestStatus& proto_status, const RequestStatus& status) {
  switch (status) {
    case RequestStatus::SUCCESSFUL:
      proto_status = proto::RequestStatus::SUCCESSFUL;
      break;
    case RequestStatus::REJECTED_USER:
      proto_status = proto::RequestStatus::REJECTED_USER;
      break;
    case RequestStatus::REJECTED_ALREADY_RUNNING:
      proto_status = proto::RequestStatus::REJECTED_ALREADY_RUNNING;
      break;
    case RequestStatus::INVALID:
      proto_status = proto::RequestStatus::INVALID;
      break;
    case RequestStatus::STOPPED:
      proto_status = proto::RequestStatus::STOPPED;
      break;
  }
}

void fromProto(const proto::RequestStatus& proto_status, RequestStatus& status) {
  switch (proto_status) {
    case proto::RequestStatus::SUCCESSFUL:
      status = RequestStatus::SUCCESSFUL;
      break;
    case proto::RequestStatus::REJECTED_USER:
      status = RequestStatus::REJECTED_USER;
      break;
    case proto::RequestStatus::REJECTED_ALREADY_RUNNING:
      status = RequestStatus::REJECTED_ALREADY_RUNNING;
      break;
    case proto::RequestStatus::INVALID:
      status = RequestStatus::INVALID;
      break;
    case proto::RequestStatus::STOPPED:
      status = RequestStatus::STOPPED;
      break;
    default:
      LOG(ERROR) << "Unknown RequestStatus: " << magic_enum::enum_name(proto_status);
      status = RequestStatus::REJECTED_USER;
  }
}

void toProto(proto::RequestResponse& proto_response, const RequestResponse& response) {
  proto::RequestStatus proto_status{};
  toProto(proto_status, response.status);
  proto_response.set_status(proto_status);
}

void fromProto(const proto::RequestResponse& proto_response, RequestResponse& response) {
  fromProto(proto_response.status(), response.status);
}
// NOLINTEND(misc-use-internal-linkage)
}  // namespace heph::ipc::zenoh::action_server
