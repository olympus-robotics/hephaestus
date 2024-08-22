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

/// We use single char key to reduce the overhead of the attachment.
[[nodiscard]] static constexpr auto messageCounterKey() -> const char* {
  return "0";
}

[[nodiscard]] static constexpr auto sessionIdKey() -> const char* {
  return "1";
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

inline auto toChrono(uint64_t ts) -> std::chrono::nanoseconds {
  const auto seconds = std::chrono::seconds{ static_cast<std::uint32_t>(ts >> 32U) };
  const auto fraction = std::chrono::nanoseconds{ static_cast<std::uint32_t>(ts & 0xFFFFFFFF) };

  return seconds + fraction;
}

inline auto toChrono(const ::zenoh::Timestamp& ts) -> std::chrono::nanoseconds {
  return toChrono(ts.get_time());
}

// template <typename T>
// constexpr auto expect(std::variant<T, ::zenoh::ErrorMessage>&& v) -> T {
//   if (std::holds_alternative<::zenoh::ErrorMessage>(v)) {
//     const auto msg = std::get<::zenoh::ErrorMessage>(v).as_string_view();
//     throwException<InvalidOperationException>(fmt::format("zenoh error: {}", msg));
//   }

//   return std::get<T>(std::move(v));
// }

// template <typename T>
// constexpr auto expectAsSharedPtr(std::variant<T, ::zenoh::ErrorMessage>&& v) -> std::shared_ptr<T> {
//   if (std::holds_alternative<::zenoh::ErrorMessage>(v)) {
//     const auto msg = std::get<::zenoh::ErrorMessage>(v).as_string_view();
//     throwException<InvalidOperationException>(fmt::format("zenoh error: {}", msg));
//   }

//   return std::make_shared<T>(std::move(std::get<T>(std::move(v))));
// }

// template <typename T>
// constexpr auto expectAsUniquePtr(std::variant<T, ::zenoh::ErrorMessage>&& v) -> std::unique_ptr<T> {
//   if (std::holds_alternative<::zenoh::ErrorMessage>(v)) {
//     const auto msg = std::get<::zenoh::ErrorMessage>(v).as_string_view();
//     throwException<InvalidOperationException>(fmt::format("zenoh error: {}", msg));
//   }

//   return std::make_unique<T>(std::move(std::get<T>(std::move(v))));
// }

[[nodiscard]] auto createZenohConfig(const Config& config) -> ::zenoh::Config;

}  // namespace heph::ipc::zenoh
