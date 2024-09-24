//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/numeric_value.h"  //NOLINT(misc-include-cleaner)

#include <cstdint>

#include "hephaestus/types/proto/numeric_value.pb.h"
#include "hephaestus/utils/exception.h"

namespace heph::types::proto {

// Protobuf supports no smaller types than int32_t.
template <>
auto toProto(NumericValue& proto_value, int8_t value) -> void {
  proto_value.set_int32_value(value);
}

// Protobuf supports no smaller types than int32_t.
template <>
auto toProto(NumericValue& proto_value, int16_t value) -> void {
  proto_value.set_int32_value(value);
}

template <>
auto toProto(NumericValue& proto_value, int32_t value) -> void {
  proto_value.set_int32_value(value);
}

template <>
auto toProto(NumericValue& proto_value, int64_t value) -> void {
  proto_value.set_int64_value(value);
}

// Protobuf supports no smaller types than uint32_t.
template <>
auto toProto(NumericValue& proto_value, uint8_t value) -> void {
  proto_value.set_uint32_value(value);
}

// Protobuf supports no smaller types than uint32_t.
template <>
auto toProto(NumericValue& proto_value, uint16_t value) -> void {
  proto_value.set_uint32_value(value);
}

template <>
auto toProto(NumericValue& proto_value, uint32_t value) -> void {
  proto_value.set_uint32_value(value);
}

template <>
auto toProto(NumericValue& proto_value, uint64_t value) -> void {
  proto_value.set_uint64_value(value);
}

template <>
auto toProto(NumericValue& proto_value, float value) -> void {
  proto_value.set_float_value(value);
}

template <>
auto toProto(NumericValue& proto_value, double value) -> void {
  proto_value.set_double_value(value);
}

// Protobuf supports no smaller types than int32_t.
template <>
auto fromProto(const NumericValue& proto_value, int8_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_int32_value(), "Expected int32 value");
  value = static_cast<int8_t>(proto_value.int32_value());
}

// Protobuf supports no smaller types than int32_t.
template <>
auto fromProto(const NumericValue& proto_value, int16_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_int32_value(), "Expected int32 value");
  value = static_cast<int16_t>(proto_value.int32_value());
}
template <>
auto fromProto(const NumericValue& proto_value, int32_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_int32_value(), "Expected int32 value");
  value = proto_value.int32_value();
}

template <>
auto fromProto(const NumericValue& proto_value, int64_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_int64_value(), "Expected int64 value");
  value = proto_value.int64_value();
}

// Protobuf supports no smaller types than uint32_t.
template <>
auto fromProto(const NumericValue& proto_value, uint8_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_uint32_value(), "Expected uint32 value");
  value = static_cast<uint8_t>(proto_value.uint32_value());
}

// Protobuf supports no smaller types than uint32_t.
template <>
auto fromProto(const NumericValue& proto_value, uint16_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_uint32_value(), "Expected uint32 value");
  value = static_cast<uint16_t>(proto_value.uint32_value());
}

template <>
auto fromProto(const NumericValue& proto_value, uint32_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_uint32_value(), "Expected uint32 value");
  value = proto_value.uint32_value();
}

template <>
auto fromProto(const NumericValue& proto_value, uint64_t& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_uint64_value(), "Expected uint64 value");
  value = proto_value.uint64_value();
}

template <>
auto fromProto(const NumericValue& proto_value, float& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_float_value(), "Expected float value");
  value = proto_value.float_value();
}

template <>
auto fromProto(const NumericValue& proto_value, double& value) -> void {
  throwExceptionIf<InvalidParameterException>(!proto_value.has_double_value(), "Expected double value");
  value = proto_value.double_value();
}

}  // namespace heph::types::proto
