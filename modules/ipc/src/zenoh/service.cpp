//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/service.h"

#include <cstddef>
#include <span>
#include <vector>

#include <fmt/core.h>

#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh::internal {

// TODO: Remove these functions once zenoh resolves the issue of changing buffers based on encoding.
static constexpr int CHANGING_BYTES = 19;

void addChangingBytes(std::vector<std::byte>& buffer) {
  auto send_buffer = std::vector<std::byte>(internal::CHANGING_BYTES, std::byte{});
  buffer.insert(buffer.begin(), internal::CHANGING_BYTES, std::byte{});
}

auto removeChangingBytes(std::span<const std::byte> buffer) -> std::span<const std::byte> {
  throwExceptionIf<InvalidDataException>(
      buffer.size() < internal::CHANGING_BYTES,
      fmt::format("Buffer size should be at least {}.", internal::CHANGING_BYTES));
  return buffer.subspan(internal::CHANGING_BYTES);
}
}  // namespace heph::ipc::zenoh::internal
