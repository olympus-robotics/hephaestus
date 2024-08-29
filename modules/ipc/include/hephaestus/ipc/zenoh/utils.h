//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <numeric>
#include <span>

#include <zenoh.hxx>
#include <zenoh/api/bytes.hxx>

#include "hephaestus/ipc/common.h"

namespace heph::ipc::zenoh {

static constexpr auto TEXT_PLAIN_ENCODING = "text/plain";
/// We use single char key to reduce the overhead of the attachment.
static constexpr auto PUBLISHER_ATTACHMENT_MESSAGE_COUNTER_KEY = "0";
static constexpr auto PUBLISHER_ATTACHMENT_MESSAGE_SESSION_ID_KEY = "1";

[[nodiscard]] static inline auto toByteVector(const ::zenoh::Bytes& bytes) -> std::vector<std::byte> {
  auto reader = bytes.reader();
  std::vector<std::byte> vec(bytes.size());
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast)
  reader.read(reinterpret_cast<uint8_t*>(vec.data()), vec.size());
  return vec;
}

[[nodiscard]] static inline auto toZenohBytes(std::span<const std::byte> buffer) -> ::zenoh::Bytes {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const std::string_view data_view{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };
  return ::zenoh::Bytes{ data_view };
}

inline auto toString(const ::zenoh::Id& id) -> std::string {
  return std::accumulate(std::begin(id.bytes()), std::end(id.bytes()), std::string(),
                         [](const std::string& s, uint8_t v) { return fmt::format("{:02x}", v) + s; });
}

constexpr auto toString(const ::zenoh::WhatAmI& me) -> std::string_view {
  switch (me) {
    case ::zenoh::WhatAmI::Z_WHATAMI_ROUTER:
      return "Router";
    case ::zenoh::WhatAmI::Z_WHATAMI_PEER:
      return "Peer";
    case ::zenoh::WhatAmI::Z_WHATAMI_CLIENT:
      return "Client";
  }
}

constexpr auto toString(const Mode& mode) -> std::string_view {
  switch (mode) {
    case Mode::ROUTER:
      return "Router";
    case Mode::PEER:
      return "Peer";
    case Mode::CLIENT:
      return "Client";
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable.
}

constexpr auto toMode(const ::zenoh::WhatAmI& me) -> Mode {
  switch (me) {
    case ::zenoh::WhatAmI::Z_WHATAMI_ROUTER:
      return Mode::ROUTER;
    case ::zenoh::WhatAmI::Z_WHATAMI_PEER:
      return Mode::PEER;
    case ::zenoh::WhatAmI::Z_WHATAMI_CLIENT:
      return Mode::CLIENT;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable.
}

inline auto toString(const std::vector<std::string>& vec) -> std::string {
  std::string str = "[";
  for (const auto& value : vec) {
    str += fmt::format("\"{:s}\", ", value);
  }

  str += "]";
  return str;
}

inline auto toChrono(uint64_t timestamp) -> std::chrono::nanoseconds {
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

inline auto toChrono(const ::zenoh::Timestamp& timestamp) -> std::chrono::nanoseconds {
  return toChrono(timestamp.get_time());
}

[[nodiscard]] auto createZenohConfig(const Config& config) -> ::zenoh::Config;

}  // namespace heph::ipc::zenoh
