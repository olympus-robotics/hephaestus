//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/utils.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>
#include <zenoh/api/bytes.hxx>
#include <zenoh/api/id.hxx>
#include <zenoh/api/timestamp.hxx>

namespace heph::ipc::zenoh {
namespace {
[[nodiscard]] auto toChrono(uint64_t timestamp) -> std::chrono::nanoseconds {
  // For details see https://zenoh.io/docs/manual/abstractions/#timestamp
  const auto seconds = std::chrono::seconds{ static_cast<std::uint32_t>(timestamp >> 32U) };
  static constexpr auto FRACTION_MASK = 0xFFFFFFF0;
  auto fraction = static_cast<uint32_t>(timestamp & FRACTION_MASK);  //
  // Convert fraction to nanoseconds
  // The fraction is in units of 2^-32 seconds, so we multiply by 10^9 / 2^32
  auto nanoseconds =
      std::chrono::nanoseconds{ static_cast<uint64_t>(fraction) * 1'000'000'000 / 0x100000000 };  // NOLINT

  return seconds + nanoseconds;
}
}  // namespace

auto toByteVector(const ::zenoh::Bytes& bytes) -> std::vector<std::byte> {
  auto reader = bytes.reader();
  std::vector<std::byte> vec(bytes.size());
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast)
  reader.read(reinterpret_cast<uint8_t*>(vec.data()), vec.size());
  return vec;
}

auto toZenohBytes(std::span<const std::byte> buffer) -> ::zenoh::Bytes {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const std::string_view data_view{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };
  return ::zenoh::Bytes{ data_view };
}

auto toString(const ::zenoh::Id& id) -> std::string {
  return std::accumulate(std::begin(id.bytes()), std::end(id.bytes()), std::string(),
                         [](const std::string& s, uint8_t v) { return fmt::format("{:02x}", v) + s; });
}

auto toString(const std::vector<std::string>& vec) -> std::string {
  std::string str = "[";
  for (const auto& value : vec) {
    str += fmt::format("\"{:s}\", ", value);
  }

  str += "]";
  return str;
}

auto toChrono(const ::zenoh::Timestamp& timestamp) -> std::chrono::nanoseconds {
  return toChrono(timestamp.get_time());
}

}  // namespace heph::ipc::zenoh
