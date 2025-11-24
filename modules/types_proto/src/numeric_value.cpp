//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/numeric_value.h"  //NOLINT(misc-include-cleaner)

#include <cstdint>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/types/proto/numeric_value.pb.h"

namespace heph::types::proto {


}  // namespace heph::types::proto

#include "hephaestus/serdes/protobuf/protobuf.inl"

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(int8_t);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(int16_t);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(int32_t);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(int64_t);

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(uint8_t);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(uint16_t);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(uint32_t);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(uint64_t);

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(float);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(double);
