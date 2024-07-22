//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>
#include <vector>

#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::serdes::protobuf {

// TODO: add option to provide custom allocator for buffer.
// TODO(filippo): I get the following compiler error, but it seems to me redundant to declare the constructor
// for empty init. Remove lint warning?
// NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
class SerializerBuffer {
public:
  template <ProtobufMessage Proto>
  void serialize(const Proto& proto) {
    buffer_.resize(proto.ByteSizeLong());
    proto.SerializeToArray(buffer_.data(), static_cast<int>(buffer_.size()));
  }

  [[nodiscard]] auto exctractSerializedData() && -> std::vector<std::byte>&& {
    return std::move(buffer_);
  }

private:
  std::vector<std::byte> buffer_;
};

class DeserializerBuffer {
public:
  explicit DeserializerBuffer(std::span<const std::byte> buffer) : buffer_(buffer) {
  }

  template <ProtobufMessage Proto>
  [[nodiscard]] auto deserialize(Proto& proto) const -> bool {
    return proto.ParseFromArray(buffer_.data(), static_cast<int>(buffer_.size()));
  }

private:
  std::span<const std::byte> buffer_;
};
// NOLINTEND(cppcoreguidelines-pro-type-member-init)

}  // namespace heph::serdes::protobuf
