//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/service.h"

namespace heph::ipc::zenoh::internal {
// reason. Remove once resolved.
auto addChangingBytes(std::vector<std::byte> buffer) -> std::vector<std::byte> {
  // NOTE: This is necessary since zenoh changes the buffer depending on the encoding for some reason.
  // Remove once resolved.
  auto send_buffer = std::vector<std::byte>(internal::CHANGING_BYTES, std::byte{});
  send_buffer.insert(send_buffer.end(), buffer.begin(), buffer.end());
  return send_buffer;
}

auto removeChangingBytes(std::span<const std::byte> buffer) -> std::span<const std::byte> {
  CHECK(buffer.size() >= internal::CHANGING_BYTES)
      << "Buffer size should be at least " << internal::CHANGING_BYTES;
  return buffer.subspan(internal::CHANGING_BYTES);
}
}  // namespace heph::ipc::zenoh::internal
