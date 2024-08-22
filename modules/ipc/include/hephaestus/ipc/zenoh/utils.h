//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <span>

#include <zenoh.hxx>

#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

/// We use single char key to reduce the overhead of the attachment.
[[nodiscard]] static constexpr auto messageCounterKey() -> const char* {
  return "0";
}

[[nodiscard]] static constexpr auto sessionIdKey() -> const char* {
  return "1";
}

[[nodiscard]] auto toString(const zenohc::Id& id) -> std::string;

[[nodiscard]] constexpr auto toString(const zenohc::WhatAmI& me) -> std::string_view {
  switch (me) {
    case zenohc::WhatAmI::Z_WHATAMI_ROUTER:
      return "Router";
    case zenohc::WhatAmI::Z_WHATAMI_PEER:
      return "Peer";
    case zenohc::WhatAmI::Z_WHATAMI_CLIENT:
      return "Client";
  }
}

[[nodiscard]] constexpr auto toString(const Mode& mode) -> std::string_view {
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

[[nodiscard]] constexpr auto toMode(const zenohc::WhatAmI& me) -> Mode {
  switch (me) {
    case zenohc::WhatAmI::Z_WHATAMI_ROUTER:
      return Mode::ROUTER;
    case zenohc::WhatAmI::Z_WHATAMI_PEER:
      return Mode::PEER;
    case zenohc::WhatAmI::Z_WHATAMI_CLIENT:
      return Mode::CLIENT;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable.
}

[[nodiscard]] auto toStringVector(const zenohc::StrArrayView& arr) -> std::vector<std::string>;

[[nodiscard]] auto toString(const std::vector<std::string>& vec) -> std::string;

[[nodiscard]] auto toChrono(uint64_t ts) -> std::chrono::nanoseconds;

[[nodiscard]] auto toChrono(const zenohc::Timestamp& ts) -> std::chrono::nanoseconds;

[[nodiscard]] auto toByteSpan(zenohc::BytesView bytes) -> std::span<const std::byte>;

template <typename T>
constexpr auto expect(std::variant<T, zenohc::ErrorMessage>&& v) -> T {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(fmt::format("zenoh error: {}", msg));
  }

  return std::get<T>(std::move(v));
}

template <typename T>
constexpr auto expectAsSharedPtr(std::variant<T, zenohc::ErrorMessage>&& v) -> std::shared_ptr<T> {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(fmt::format("zenoh error: {}", msg));
  }

  return std::make_shared<T>(std::move(std::get<T>(std::move(v))));
}

template <typename T>
constexpr auto expectAsUniquePtr(std::variant<T, zenohc::ErrorMessage>&& v) -> std::unique_ptr<T> {
  if (std::holds_alternative<zenohc::ErrorMessage>(v)) {
    const auto msg = std::get<zenohc::ErrorMessage>(v).as_string_view();
    throwException<InvalidOperationException>(fmt::format("zenoh error: {}", msg));
  }

  return std::make_unique<T>(std::move(std::get<T>(std::move(v))));
}

}  // namespace heph::ipc::zenoh
