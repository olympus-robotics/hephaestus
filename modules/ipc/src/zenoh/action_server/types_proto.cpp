//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <absl/log/log.h>
#include <magic_enum.hpp>

#include "hephaestus/ipc/zenoh/action_server/proto/types.pb.h"
#include "hephaestus/ipc/zenoh/action_server/types.h"
#include "hephaestus/serdes/protobuf/enums.h"

namespace heph::ipc::zenoh::action_server {
// TODO(@filippobrizzi): for some reason clang-tidy doesn't understand that this are public function declared
// in the header.
/// NOLINTBEGIN(misc-use-internal-linkage)

void toProto(proto::RequestResponse& proto_response, const RequestResponse& response) {
  proto_response.set_status(serdes::protobuf::toProtoEnum<proto::RequestStatus>(response.status));
}

void fromProto(const proto::RequestResponse& proto_response, RequestResponse& response) {
  serdes::protobuf::fromProto(proto_response.status(), response.status);
}
// NOLINTEND(misc-use-internal-linkage)
}  // namespace heph::ipc::zenoh::action_server
