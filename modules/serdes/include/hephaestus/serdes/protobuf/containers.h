//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <type_traits>
#include <vector>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/repeated_ptr_field.h>

//=================================================================================================
// NOTE: In order for namespace resolution to work correctly for container serialization introduced
// in this file, calls to toProto and fromProto must be prefixed with (heph::)serdes::protobuf::
//=================================================================================================

namespace heph::serdes::protobuf {

//=================================================================================================
// Vector (std::vector <-> google::protobuf::RepeatedField)
//=================================================================================================
template <typename T, typename ProtoT>
  requires std::is_arithmetic_v<T>
auto toProto(google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, const std::vector<T>& vec)
    -> void {
  proto_repeated_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_field.Reserve(static_cast<int>(vec.size()));
  proto_repeated_field.Add(vec.begin(), vec.end());
}

template <typename T, typename ProtoT>
auto toProto(google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field, const std::vector<T>& vec)
    -> void {
  proto_repeated_ptr_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_ptr_field.Reserve(static_cast<int>(vec.size()));
  for (const auto& value : vec) {
    toProto(*(proto_repeated_ptr_field.Add()), value);
  }
}

template <typename T, typename ProtoT>
  requires std::is_arithmetic_v<T>
auto fromProto(const google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, std::vector<T>& vec)
    -> void {
  vec.clear();  // Ensure that the vector is empty before adding elements.
  vec.reserve(static_cast<size_t>(proto_repeated_field.size()));
  std::transform(proto_repeated_field.begin(), proto_repeated_field.end(), std::back_inserter(vec),
                 [](const auto& proto_value) { return static_cast<T>(proto_value); });
}

template <typename T, typename ProtoT>
auto fromProto(const google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field,
               std::vector<T>& vec) -> void {
  vec.clear();  // Ensure that the vector is empty before adding elements.
  vec.reserve(static_cast<size_t>(proto_repeated_ptr_field.size()));
  std::transform(proto_repeated_ptr_field.begin(), proto_repeated_ptr_field.end(), std::back_inserter(vec),
                 [](const auto& proto_value) {
                   T value;
                   fromProto(proto_value, value);
                   return value;
                 });
}

}  // namespace heph::serdes::protobuf
