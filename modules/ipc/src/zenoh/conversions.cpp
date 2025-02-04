//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/conversions.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
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

[[nodiscard]] auto fromAsciiHex(std::span<const uint8_t> input) -> std::string {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto str = std::string{ reinterpret_cast<const char*>(input.data()), input.size() };
  str.erase(std::ranges::find(str, '\0'), str.end());
  return str;
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

/// Only alphanumeric or underscore characters are allowed.
auto isValidIdChar(char c) -> bool {
  return std::isalnum(c) != 0 || c == '_';
}

auto isValidId(std::string_view session_id) -> bool {
  return std::ranges::all_of(session_id, [](char c) { return isValidIdChar(c); });
}

auto toString(const ::zenoh::Id& id) -> std::string {
  auto id_str = fromAsciiHex(std::span<const uint8_t>{ id.bytes().data(), id.bytes().size() });
  if (!id_str.empty() && isValidId(id_str)) {
    return id_str;
  }

  return id.to_string();
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
