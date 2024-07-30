//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::serdes {

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string;

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void;

template <class T>
auto deserializeFromText(std::string_view buffer, T& data) -> void;

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
auto deserialize(std::span<const std::byte> buffer, T& data) -> void {
  protobuf::deserialize(buffer, data);
}

template <protobuf::ProtobufSerializable T>
auto deserializeFromText(std::string_view buffer, T& data) -> void {
  protobuf::deserializeFromText(buffer, data);
}

template <protobuf::ProtobufSerializable T>
auto getSerializedTypeInfo() -> TypeInfo {
  return protobuf::getTypeInfo<T>();
}

}  // namespace heph::serdes
