//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>

namespace heph::serdes::protobuf {

template <class T>
struct ProtoAssociation {};

template <typename T>
concept ProtobufMessage = requires(T proto, void* out_data, const void* in_data, int size) {
  { proto.SerializeToArray(out_data, size) };
  { proto.ParseFromArray(in_data, size) } -> std::same_as<bool>;
};

template <class Proto, class T>
concept ProtobufConvertible = requires(T data, Proto proto) {
  { toProto(proto, data) };
  { fromProto(proto, data) };
};

template <class T>
concept ProtobufSerializable = protobuf::ProtobufMessage<typename protobuf::ProtoAssociation<T>::Type>;

}  // namespace heph::serdes::protobuf
