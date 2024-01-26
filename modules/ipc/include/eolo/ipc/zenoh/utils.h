//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <expected>
#include <numeric>
#include <span>
#include <zenohc.hxx>

#include "eolo/base/exception.h"
#include "eolo/ipc/common.h"
namespace eolo::ipc::zenoh {

[[nodiscard]] inline static constexpr auto messageCounterKey() -> const char* {
  return "msg_counter";
}

[[nodiscard]] inline static constexpr auto sessionIdKey() -> const char* {
  return "session_id";
}

inline auto toString(const zenohc::Id& id) -> std::string {
  return std::accumulate(std::begin(id.id), std::end(id.id), std::string(),
                         [](const std::string& s, uint8_t v) { return std::format("{:02x}", v) + s; });
}

inline constexpr auto toString(const zenohc::WhatAmI& me) -> std::string_view {
  switch (me) {
    case zenohc::WhatAmI::Z_WHATAMI_ROUTER:
      return "Router";
    case zenohc::WhatAmI::Z_WHATAMI_PEER:
      return "Peer";
    case zenohc::WhatAmI::Z_WHATAMI_CLIENT:
      return "Client";
  }
}

inline constexpr auto toString(const Mode& mode) -> std::string_view {
  switch (mode) {
    case Mode::ROUTER:
      return "Router";
    case Mode::PEER:
      return "Peer";
    case Mode::CLIENT:
      return "Client";
  }
}

inline constexpr auto toMode(const zenohc::WhatAmI& me) -> Mode {
  switch (me) {
    case zenohc::WhatAmI::Z_WHATAMI_ROUTER:
      return Mode::ROUTER;
    case zenohc::WhatAmI::Z_WHATAMI_PEER:
      return Mode::PEER;
    case zenohc::WhatAmI::Z_WHATAMI_CLIENT:
      return Mode::CLIENT;
  }
}

inline auto toStringVector(const zenohc::StrArrayView& arr) -> std::vector<std::string> {
  const auto size = arr.get_len();
  std::vector<std::string> vec;
  vec.reserve(size);

  for (size_t i = 0; i < size; ++i) {
    vec.emplace_back(std::format("{:s}", arr[i]));
  }

  return vec;
}

inline auto toString(const std::vector<std::string>& vec) -> std::string {
  std::string str = "[";
  for (const auto& value : vec) {
    str += std::format("\"{:s}\", ", value);
  }

  str += "]";
  return str;
}

inline auto toChrono(uint64_t ts) -> std::chrono::nanoseconds {
  const auto seconds = std::chrono::seconds{ static_cast<std::uint32_t>(ts >> 32U) };
  const auto fraction = std::chrono::nanoseconds{ static_cast<std::uint32_t>(ts & 0xFFFFFFFF) };

  return seconds + fraction;
}

inline auto toChrono(const zenohc::Timestamp& ts) -> std::chrono::nanoseconds {
  return toChrono(ts.get_time());
}

template <typename T>
inline constexpr auto expect(std::variant<T, zenohc::ErrorMessage>&& v) -> T {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(std::format("zenoh error: {}", msg));
  }

  return std::get<T>(std::move(v));
}

template <typename T>
inline constexpr auto expectAsSharedPtr(std::variant<T, zenohc::ErrorMessage>&& v) -> std::shared_ptr<T> {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(std::format("zenoh error: {}", msg));
  }

  return std::make_shared<T>(std::move(std::get<T>(std::move(v))));
}

template <typename T>
inline constexpr auto expectAsUniquePtr(std::variant<T, zenohc::ErrorMessage>&& v) -> std::unique_ptr<T> {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(std::format("zenoh error: {}", msg));
  }

  return std::make_unique<T>(std::move(std::get<T>(std::move(v))));
}

inline auto toByteSpan(zenohc::BytesView bytes) -> std::span<const std::byte> {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return { reinterpret_cast<const std::byte*>(bytes.as_string_view().data()), bytes.as_string_view().size() };
}

[[nodiscard]] auto createZenohConfig(const Config& config) -> zenohc::Config;

}  // namespace eolo::ipc::zenoh
