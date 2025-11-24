//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/bounds.h"  // NOLINT(misc-include-cleaner)

#include "hephaestus/serdes/protobuf/protobuf.inl"

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<char>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<short>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<int>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<long>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<long long>);

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<unsigned char>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<unsigned short>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<unsigned int>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<unsigned long>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<unsigned long long>);

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<float>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<double>);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::Bounds<long double>);
