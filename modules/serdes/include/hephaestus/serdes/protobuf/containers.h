//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <vector>

#include <google/protobuf/repeated_field.h>

//=================================================================================================
// NOTE: In order for namespace resolution to work correctly for container serialization introduced
// in this file, calls to toProto and fromProto must be prefixed with (heph::)serdes::protobuf::
//=================================================================================================

namespace heph::serdes::protobuf {

//=================================================================================================
// Vector (std::vector <-> google::protobuf::RepeatedField)
//=================================================================================================
template <typename T, typename ProtoT>
auto toProto(google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, const std::vector<T>& vec)
    -> void {
  proto_repeated_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_field.Reserve(static_cast<int>(vec.size()));
  for (const auto& value : vec) {
    auto* proto_value = proto_repeated_field.Add();
    toProto(*proto_value, value);
  }
}

template <typename T, typename ProtoT>
auto toProto(google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field, const std::vector<T>& vec)
    -> void {
  proto_repeated_ptr_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_ptr_field.Reserve(static_cast<int>(vec.size()));
  for (const auto& value : vec) {
    auto* proto_value = proto_repeated_ptr_field.Add();
    toProto(*proto_value, value);
  }
}

template <typename T, typename ProtoT>
auto fromProto(const google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, std::vector<T>& vec)
    -> void {
  vec.clear();  // Ensure that the vector is empty before adding elements.
  vec.resize(static_cast<size_t>(proto_repeated_field.size()));
  size_t idx = 0;
  for (const auto& proto_value : proto_repeated_field) {
    fromProto(proto_value, vec[idx++]);
  }
}

template <typename T, typename ProtoT>
auto fromProto(const google::protobuf::RepeatedPtrField<ProtoT>& proto_repeated_ptr_field,
               std::vector<T>& vec) -> void {
  vec.clear();  // Ensure that the vector is empty before adding elements.
  vec.resize(static_cast<size_t>(proto_repeated_ptr_field.size()));
  size_t idx = 0;
  for (const auto& proto_value : proto_repeated_ptr_field) {
    fromProto(proto_value, vec[idx++]);
  }
}

}  // namespace heph::serdes::protobuf
