//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/containers/proto/bit_flag.pb.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/enums.h"

namespace heph::serdes::protobuf {
template <typename EnumT>
struct ProtoAssociation<containers::BitFlag<EnumT>> {
  using Type = containers::proto::BitFlag;
};
}  // namespace heph::serdes::protobuf

namespace heph::containers {

template <typename EnumT>
auto toProto(proto::BitFlag& proto_bit_flag, const BitFlag<EnumT>& bit_flag) -> void {
  proto_bit_flag.set_value(bit_flag.getUnderlyingValue());
}

template <typename EnumT>
auto fromProto(const proto::BitFlag& proto_bit_flag, BitFlag<EnumT>& bit_flag) -> void {
  bit_flag = BitFlag<EnumT>{ static_cast<BitFlag<EnumT>::T>(proto_bit_flag.value()) };
}

}  // namespace heph::containers
