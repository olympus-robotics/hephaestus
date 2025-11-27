//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/containers/proto/bit_flag.pb.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::serdes::protobuf {
template <typename EnumT>
struct ProtoAssociation<containers::BitFlag<EnumT>> {
  using Type = containers::proto::BitFlag;
};
}  // namespace heph::serdes::protobuf

namespace heph::containers {

template <typename EnumT>
void toProto(proto::BitFlag& proto_bit_flag, const BitFlag<EnumT>& bit_flag) {
  proto_bit_flag.set_value(bit_flag.getUnderlyingValue());
}

template <typename EnumT>
void fromProto(const proto::BitFlag& proto_bit_flag, BitFlag<EnumT>& bit_flag) {
  const auto bit_flag_value =
      BitFlag<EnumT>::fromUnderlyingValue(static_cast<BitFlag<EnumT>::T>(proto_bit_flag.value()));
  HEPH_PANIC_IF(!bit_flag_value.has_value(),
                "Failed to deserialize BitFlag from protobuf: underlying value contains invalid bits.");
  bit_flag = bit_flag_value.value();
}

}  // namespace heph::containers
