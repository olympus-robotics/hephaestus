//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/bool.pb.h"
#include "hephaestus/utils/concepts.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<bool> {
  using Type = types::proto::Bool;
};
}  // namespace heph::serdes::protobuf

namespace heph::types::proto {

/// \brief Convert a numeric value to a protobuf message. These functions are specialized for each numeric
/// type. The main usage is to allow for serialization of templated numeric types.
auto toProto(Bool& proto_value, bool value) -> void;
auto fromProto(const Bool& proto_value, bool& value) -> void;

}  // namespace heph::types::proto
