//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/containers/proto/bit_flag.pb.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"
#include "hephaestus/serdes/type_info.h"

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
  panicIf(!bit_flag_value.has_value(),
          "Failed to deserialize BitFlag from protobuf: underlying value contains invalid bits.");
  bit_flag = bit_flag_value.value();
}

}  // namespace heph::containers

namespace heph::serdes::protobuf {

template <typename EnumT>
auto serialize(const heph::containers::BitFlag<EnumT>& data) -> std::vector<std::byte> {
  return serialize(data.getUnderlyingValue());
}

template <typename EnumT>
[[nodiscard]] auto serializeToJSON(const heph::containers::BitFlag<EnumT>& data) -> std::string {
  return serializeToJSON(data.getUnderlyingValue());
}

template <typename EnumT>
[[nodiscard]] auto serializeToText(const heph::containers::BitFlag<EnumT>& data) -> std::string {
  return serializeToText(data.getUnderlyingValue());
}

template <typename EnumT>
void deserialize(std::span<const std::byte> buffer, heph::containers::BitFlag<EnumT>& data) {
  deserialize(buffer, data.getUnderlyingValue());
}

template <typename EnumT>
void deserializeFromJSON(std::string_view buffer, heph::containers::BitFlag<EnumT>& data) {
  deserializeFromJSON(buffer, data.getUnderlyingValue());
}

template <typename EnumT>
void deserializeFromText(std::string_view buffer, heph::containers::BitFlag<EnumT>& data) {
  deserializeFromText(buffer, data.getUnderlyingValue());
}

template <typename BitFlagT>
auto getTypeInfo()
    -> TypeInfo
  requires requires { typename BitFlagT::EnumT; }
{
  return getTypeInfo<BitFlagT>();
}

}  // namespace heph::serdes::protobuf
