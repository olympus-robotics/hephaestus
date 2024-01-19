//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <expected>
#include <span>
#include <sstream>
#include <zenohc.hxx>

#include "eolo/base/exception.h"

namespace eolo::ipc {
inline auto toString(const zenohc::Id& id) -> std::string {
  std::stringstream ss;
  ss << id;
  return ss.str();
}

template <typename T>
inline constexpr auto expect(std::variant<T, zenohc::ErrorMessage>&& v) -> T {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(std::format("zenoh error: {}", msg));
  }

  return std::get<T>(std::move(v));
}

inline auto toByteSpan(zenohc::BytesView bytes) -> std::span<const std::byte> {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return { reinterpret_cast<const std::byte*>(bytes.as_string_view().data()),
           bytes.as_string_view().size() };
}

}  // namespace eolo::ipc
