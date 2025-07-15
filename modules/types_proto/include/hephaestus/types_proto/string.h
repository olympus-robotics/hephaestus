//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/string.pb.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<std::string> {
  using Type = types::proto::String;
};
}  // namespace heph::serdes::protobuf

namespace heph::types::proto {
void toProto(String& proto_value, std::string value);
void fromProto(const String& proto_value, std::string& value);

}  // namespace heph::types::proto
