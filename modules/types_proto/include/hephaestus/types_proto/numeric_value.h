//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/numeric_value.pb.h"

template <typename T>
concept IsNumeric = std::integral<T> || std::floating_point<T>;

namespace heph::serdes::protobuf {
template <IsNumeric T>
struct ProtoAssociation<T> {
  using Type = types::proto::NumericValue;
};
}  // namespace heph::serdes::protobuf

namespace heph::types::proto {

template <IsNumeric T>
auto toProto(NumericValue& proto_value, const T value) -> void;
template <IsNumeric T>
auto fromProto(const NumericValue& proto_value, T& value) -> void;

}  // namespace heph::types::proto