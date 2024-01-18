//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <concepts>

namespace eolo::serdes::protobuf {
template <typename T>
concept ProtobufMessage = requires(T proto, void* out_data, const void* in_data, int size) {
  { proto.SerializeToArray(out_data, size) };
  { proto.ParseFromArray(in_data, size) } -> std::same_as<bool>;
};

}  // namespace eolo::serdes::protobuf
