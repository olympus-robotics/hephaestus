//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/string.h"

#include "hephaestus/types/proto/string.pb.h"

namespace heph::types::proto {

void toProto(String& proto_value, std::string value) {
  proto_value.set_value(value);
}

void fromProto(const String& proto_value, std::string& value) {
  value = proto_value.value();
}

}  // namespace heph::types::proto
