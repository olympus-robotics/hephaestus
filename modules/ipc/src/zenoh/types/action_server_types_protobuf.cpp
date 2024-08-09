//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <absl/log/log.h>
#include <magic_enum.hpp>

#include "hephaestus/ipc/zenoh/proto/action_server_types.pb.h"
#include "hephaestus/ipc/zenoh/types/action_server_types.h"

namespace heph::ipc::zenoh {
void toProto(proto::ActionServerRequestStatus& proto_status, const ActionServerRequestStatus& status) {
  switch (status) {
    case ActionServerRequestStatus::SUCCESSFUL:
      proto_status = proto::ActionServerRequestStatus::SUCCESSFUL;
      break;
    case ActionServerRequestStatus::REJECTED_USER:
      proto_status = proto::ActionServerRequestStatus::REJECTED_USER;
      break;
    case ActionServerRequestStatus::REJECTED_ALREADY_RUNNING:
      proto_status = proto::ActionServerRequestStatus::REJECTED_ALREADY_RUNNING;
      break;
    case ActionServerRequestStatus::INVALID:
      proto_status = proto::ActionServerRequestStatus::INVALID;
      break;
    case ActionServerRequestStatus::STOPPED:
      proto_status = proto::ActionServerRequestStatus::STOPPED;
      break;
  }
}

void fromProto(const proto::ActionServerRequestStatus& proto_status, ActionServerRequestStatus& status) {
  switch (proto_status) {
    case proto::ActionServerRequestStatus::SUCCESSFUL:
      status = ActionServerRequestStatus::SUCCESSFUL;
      break;
    case proto::ActionServerRequestStatus::REJECTED_USER:
      status = ActionServerRequestStatus::REJECTED_USER;
      break;
    case proto::ActionServerRequestStatus::REJECTED_ALREADY_RUNNING:
      status = ActionServerRequestStatus::REJECTED_ALREADY_RUNNING;
      break;
    case proto::ActionServerRequestStatus::INVALID:
      status = ActionServerRequestStatus::INVALID;
      break;
    case proto::ActionServerRequestStatus::STOPPED:
      status = ActionServerRequestStatus::STOPPED;
      break;
    default:
      LOG(ERROR) << "Unknown ActionServerRequestStatus: " << magic_enum::enum_name(proto_status);
      status = ActionServerRequestStatus::REJECTED_USER;
  }
}

void toProto(proto::ActionServerRequestResponse& proto_response,
             const ActionServerRequestResponse& response) {
  proto::ActionServerRequestStatus proto_status{};
  toProto(proto_status, response.status);
  proto_response.set_status(proto_status);
}

void fromProto(const proto::ActionServerRequestResponse& proto_response,
               ActionServerRequestResponse& response) {
  fromProto(proto_response.status(), response.status);
}
}  // namespace heph::ipc::zenoh
