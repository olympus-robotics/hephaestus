//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <expected>
#include <span>
#include <sstream>
#include <zenohc.hxx>

#include "eolo/base/exception.h"

namespace eolo::ipc::zenoh {

[[nodiscard]] inline static constexpr auto messageCounterKey() -> const char* {
  return "msg_counter";
}

[[nodiscard]] inline static constexpr auto sessionIdKey() -> const char* {
  return "session_id";
}

inline auto toString(const zenohc::Id& id) -> std::string {
  std::stringstream ss;
  ss << id;
  return ss.str();
}

inline auto toChrono(uint64_t ts) -> std::chrono::nanoseconds {
  const auto seconds = std::chrono::seconds{ static_cast<std::uint32_t>(ts >> 32U) };
  const auto fraction = std::chrono::nanoseconds{ static_cast<std::uint32_t>(ts & 0xFFFFFFFF) };

  return seconds + fraction;
}

inline auto toChrono(const zenohc::Timestamp& ts) -> std::chrono::nanoseconds {
  std::println("Converting timestamp: {}, is valid: {}", ts.get_time(), ts.check());
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
inline constexpr auto expectAsPtr(std::variant<T, zenohc::ErrorMessage>&& v) -> std::unique_ptr<T> {
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

}  // namespace eolo::ipc::zenoh
