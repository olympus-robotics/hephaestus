//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace heph::serdes {
struct TypeInfo;
}

namespace heph::serdes::protobuf {

template <class T>
struct ProtoAssociation;

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string;

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string;

template <class T>
void deserialize(std::span<const std::byte> buffer, T& data);

template <class T>
void deserializeFromJSON(std::string_view buffer, T& data);

template <class T>
void deserializeFromText(std::string_view buffer, T& data);

/// Create the type info for the serialized type associated with `T`.
template <class T>
[[nodiscard]] auto getTypeInfo() -> TypeInfo;

}  // namespace heph::serdes::protobuf
