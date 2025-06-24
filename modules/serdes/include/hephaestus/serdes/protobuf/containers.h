//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <cstddef>
#include <ranges>  // NOLINT(misc-include-cleaner)
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/repeated_ptr_field.h>

#include "hephaestus/utils/exception.h"

//=================================================================================================
// NOTE: In order for namespace resolution to work correctly for container serialization introduced
// in this file, calls to toProto and fromProto must be prefixed with (heph::)serdes::protobuf::
//=================================================================================================

namespace heph::serdes::protobuf {

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

//=================================================================================================
// Trivial type (value <-> proto value)
//=================================================================================================
template <Arithmetic T, typename ProtoT>
void toProto(ProtoT& proto_value, const T value) {
  proto_value = static_cast<ProtoT>(value);
}

template <Arithmetic T, typename ProtoT>
void fromProto(const ProtoT& proto_value, T& value) {
  value = static_cast<T>(proto_value);
}

//=================================================================================================
// Vector (std::vector <-> google::protobuf::RepeatedField)
//=================================================================================================
template <Arithmetic T, typename ProtoT>
void toProto(google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, const std::vector<T>& vec) {
  proto_repeated_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_field.Reserve(static_cast<int>(vec.size()));
  proto_repeated_field.Add(vec.cbegin(), vec.cend());
}

template <typename T, typename ProtoT>
void toProto(google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field,
             const std::vector<T>& vec) {
  proto_repeated_ptr_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_ptr_field.Reserve(static_cast<int>(vec.size()));
  for (const auto& value : vec) {
    toProto(*(proto_repeated_ptr_field.Add()), value);
  }
}

template <Arithmetic T, typename ProtoT>
void fromProto(const google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, std::vector<T>& vec) {
  vec.clear();  // Ensure that the vector is empty before adding elements.
  vec.reserve(static_cast<size_t>(proto_repeated_field.size()));
  // NOLINTNEXTLINE(misc-include-cleaner)
  std::ranges::transform(proto_repeated_field, std::back_inserter(vec),
                         [](const auto& proto_value) { return static_cast<T>(proto_value); });
}

template <typename T, typename ProtoT>
void fromProto(const google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field,
               std::vector<T>& vec) {
  vec.clear();  // Ensure that the vector is empty before adding elements.
                // NOLINTNEXTLINE(misc-include-cleaner)
  vec.reserve(static_cast<size_t>(proto_repeated_ptr_field.size()));
  std::ranges::transform(proto_repeated_ptr_field, std::back_inserter(vec), [](const auto& proto_value) {
    T value;
    fromProto(proto_value, value);
    return value;
  });
}

//=================================================================================================
// Array (std::array <-> google::protobuf::RepeatedField)
//=================================================================================================
template <Arithmetic T, typename ProtoT, std::size_t N>
void toProto(google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, const std::array<T, N>& arr) {
  proto_repeated_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_field.Reserve(static_cast<int>(arr.size()));
  proto_repeated_field.Add(arr.cbegin(), arr.cend());
}

template <typename T, typename ProtoT, std::size_t N>
void toProto(google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field,
             const std::array<T, N>& arr) {
  proto_repeated_ptr_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_ptr_field.Reserve(static_cast<int>(arr.size()));
  for (const auto& value : arr) {
    toProto(*(proto_repeated_ptr_field.Add()), value);
  }
}

template <Arithmetic T, typename ProtoT, std::size_t N>
void fromProto(const google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, std::array<T, N>& arr) {
  panicIf(proto_repeated_field.size() != N,
          "Mismatch between size of repeated proto field {} and size of array {}.",
          proto_repeated_field.size(), N);
  // NOLINTNEXTLINE(misc-include-cleaner)
  std::ranges::transform(proto_repeated_field, arr.begin(),
                         [](const auto& proto_value) { return static_cast<T>(proto_value); });
}

template <typename T, typename ProtoT, std::size_t N>
void fromProto(const google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field,
               std::array<T, N>& arr) {
  panicIf(proto_repeated_ptr_field.size() != N,
          "Mismatch between size of repeated proto ptr field {} and size of array {}.",
          proto_repeated_ptr_field.size(), N);
  // NOLINTNEXTLINE(misc-include-cleaner)
  std::ranges::transform(proto_repeated_ptr_field, arr.begin(), [](const auto& proto_value) {
    T value;
    fromProto(proto_value, value);
    return value;
  });
}

//=================================================================================================
// Unordered map (std::unordered_map <-> google::protobuf::RepeatedField)
//=================================================================================================
template <typename K, typename V, typename ProtoK, typename ProtoV>
void toProto(google::protobuf::Map<ProtoK, ProtoV>& proto_map, const std::unordered_map<K, V>& umap) {
  proto_map.clear();  // Ensure that the map is empty before adding elements.
  for (const auto& [key, value] : umap) {
    std::remove_const_t<ProtoK> proto_key;
    std::remove_const_t<ProtoV> proto_value;
    toProto(proto_key, key);
    toProto(proto_value, value);
    proto_map.emplace(std::move(proto_key), std::move(proto_value));
  }
}

template <typename K, typename V, typename ProtoK, typename ProtoV>
void fromProto(const google::protobuf::Map<ProtoK, ProtoV>& proto_map, std::unordered_map<K, V>& umap) {
  umap.clear();  // Ensure that the map is empty before adding elements.
  umap.reserve(static_cast<size_t>(proto_map.size()));
  for (const auto& proto_pair : proto_map) {
    std::remove_const_t<K> key;
    std::remove_const_t<V> value;
    const auto& [proto_key, proto_value] = proto_pair;
    fromProto(proto_key, key);
    fromProto(proto_value, value);
    umap.emplace(std::move(key), std::move(value));
  }
}

}  // namespace heph::serdes::protobuf
