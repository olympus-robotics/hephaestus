//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/concepts.h"

namespace heph::serdes {
/// Serdes module provides generic support for serialization and deserialization of data types.
/// It is built to support any serialization library via template specialization.
/// Right now it only support Protobuf, but can be easily extended to support other libraries by simply
/// writing a new concept for your serialization library.

template <typename T>
[[nodiscard]] auto serialize([[maybe_unused]] const T& data) -> std::vector<std::byte> {
  static_assert(false, "serialize is not implemented for this type, did you forget to include the header "
                       "with the serialization implementation?");
  __builtin_unreachable();
}

template <typename T>
[[nodiscard]] auto serializeToText([[maybe_unused]] const T& data) -> std::string {
  static_assert(false, "serialize is not implemented for this type, did you forget to include the header "
                       "with the serialization implementation?");
  __builtin_unreachable();
}

template <typename T>
void deserialize([[maybe_unused]] std::span<const std::byte> buffer, [[maybe_unused]] T& data) {
  static_assert(false, "serialize is not implemented for this type, did you forget to include the header "
                       "with the serialization implementation?");
  __builtin_unreachable();
}

template <typename T>
void deserializeFromText([[maybe_unused]] std::string_view buffer, [[maybe_unused]] T& data) {
  static_assert(false, "serialize is not implemented for this type, did you forget to include the header "
                       "with the serialization implementation?");
  __builtin_unreachable();
}

template <typename T>
auto getSerializedTypeInfo() -> TypeInfo {
  static_assert(false, "serialize is not implemented for this type, did you forget to include the header "
                       "with the serialization implementation?");
  __builtin_unreachable();
}

// -----------------------------------------------------------------------------------------------
// Specializations
// -----------------------------------------------------------------------------------------------

// -- Protobuf -----------------------------------------------------------------------------------

template <protobuf::ProtobufSerializable T>
auto serialize(const T& data) -> std::vector<std::byte> {
  return protobuf::serialize(data);
}

template <protobuf::ProtobufSerializable T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string {
  return protobuf::serializeToText(data);
}

template <protobuf::ProtobufSerializable T>
void deserialize(std::span<const std::byte> buffer, T& data) {
  protobuf::deserialize(buffer, data);
}

template <protobuf::ProtobufSerializable T>
void deserializeFromText(std::string_view buffer, T& data) {
  protobuf::deserializeFromText(buffer, data);
}

template <protobuf::ProtobufSerializable T>
auto getSerializedTypeInfo() -> TypeInfo {
  return protobuf::getTypeInfo<T>();
}

template <StringType T>
auto getSerializedTypeInfo() -> TypeInfo {
  return {
    .name = "string",
    .schema = {},
    .serialization = TypeInfo::Serialization::TEXT,
    .original_type = "string",
  };
}

}  // namespace heph::serdes
