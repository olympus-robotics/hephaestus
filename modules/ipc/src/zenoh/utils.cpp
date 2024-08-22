//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/utils.h"

#include <cstddef>
#include <iterator>
#include <numeric>
#include <span>
#include <string>
#include <vector>

#include <fmt/core.h>
// #include <zenohcxx/api.hxx>

namespace heph::ipc::zenoh {
auto toString(const zenohc::Id& id) -> std::string {
  return std::accumulate(std::begin(id.id), std::end(id.id), std::string(),
                         [](const std::string& s, uint8_t v) { return fmt::format("{:02x}", v) + s; });
}

auto toStringVector(const zenohc::StrArrayView& arr) -> std::vector<std::string> {
  const auto size = arr.get_len();
  std::vector<std::string> vec;
  vec.reserve(size);

  for (size_t i = 0; i < size; ++i) {
    vec.emplace_back(fmt::format("{:s}", arr[i]));
  }

  return vec;
}

auto toString(const std::vector<std::string>& vec) -> std::string {
  std::string str = "[";
  for (const auto& value : vec) {
    str += fmt::format("\"{:s}\", ", value);
  }

  str += "]";
  return str;
}

auto toChrono(uint64_t ts) -> std::chrono::nanoseconds {
  const auto seconds = std::chrono::seconds{ static_cast<std::uint32_t>(ts >> 32U) };
  const auto fraction = std::chrono::nanoseconds{ static_cast<std::uint32_t>(ts & 0xFFFFFFFF) };

  return seconds + fraction;
}

auto toChrono(const zenohc::Timestamp& ts) -> std::chrono::nanoseconds {
  return toChrono(ts.get_time());
}

auto toByteSpan(zenohc::BytesView bytes) -> std::span<const std::byte> {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return { reinterpret_cast<const std::byte*>(bytes.as_string_view().data()), bytes.as_string_view().size() };
}

}  // namespace heph::ipc::zenoh
