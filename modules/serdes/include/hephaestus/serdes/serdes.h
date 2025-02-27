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

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string;

template <class T>
void deserialize(std::span<const std::byte> buffer, T& data);

template <class T>
void deserializeFromText(std::string_view buffer, T& data);

template <class T>
auto getSerializedTypeInfo() -> TypeInfo;

// -----------------------------------------------------------------------------------------------
// Specialization
// -----------------------------------------------------------------------------------------------

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
