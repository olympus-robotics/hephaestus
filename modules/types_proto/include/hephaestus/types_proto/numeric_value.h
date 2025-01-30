//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/numeric_value.pb.h"
#include "hephaestus/utils/concepts.h"

namespace heph::serdes::protobuf {
template <NumericType T>
struct ProtoAssociation<T> {
  using Type = types::proto::NumericValue;
};
}  // namespace heph::serdes::protobuf

namespace heph::types::proto {

/// \brief Convert a numeric value to a protobuf message. These functions are specialized for each numeric
/// type. The main usage is to allow for serialization of templated numeric types.
template <NumericType T>
void toProto(NumericValue& proto_value, T value);
template <NumericType T>
void fromProto(const NumericValue& proto_value, T& value);

}  // namespace heph::types::proto
