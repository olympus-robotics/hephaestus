//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/examples/types/accumulator.h"
#include "hephaestus/examples/types/proto/accumulator.pb.h"
#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::examples::types::Accumulator> {
  using Type = heph::examples::types::proto::Accumulator;
};
}  // namespace heph::serdes::protobuf

namespace heph::examples::types {
inline void toProto(proto::Accumulator& proto_accumulator, const Accumulator& accumulator) {
  proto_accumulator.set_value(accumulator.value);
  proto_accumulator.set_counter(accumulator.counter);
}

inline void fromProto(const proto::Accumulator& proto_accumulator, Accumulator& accumulator) {
  accumulator.value = proto_accumulator.value();
  accumulator.counter = proto_accumulator.counter();
}

}  // namespace heph::examples::types
