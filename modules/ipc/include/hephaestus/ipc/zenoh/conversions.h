//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <zenoh/api/bytes.hxx>
#include <zenoh/api/enums.hxx>
#include <zenoh/api/id.hxx>
#include <zenoh/api/timestamp.hxx>

#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

static constexpr auto TEXT_PLAIN_ENCODING = "text/plain";
/// We use single char key to reduce the overhead of the attachment.
static constexpr auto PUBLISHER_ATTACHMENT_MESSAGE_COUNTER_KEY = "0";
static constexpr auto PUBLISHER_ATTACHMENT_MESSAGE_SESSION_ID_KEY = "1";
static constexpr auto PUBLISHER_ATTACHMENT_MESSAGE_TYPE_INFO = "2";

[[nodiscard]] auto toByteVector(const ::zenoh::Bytes& bytes) -> std::vector<std::byte>;

[[nodiscard]] auto toZenohBytes(std::span<const std::byte> buffer) -> ::zenoh::Bytes;

[[nodiscard]] auto isValidIdChar(char c) -> bool;
[[nodiscard]] auto isValidId(std::string_view session_id) -> bool;
[[nodiscard]] auto toString(const ::zenoh::Id& id) -> std::string;

[[nodiscard]] auto toString(const std::vector<std::string>& vec) -> std::string;

[[nodiscard]] auto toChrono(const ::zenoh::Timestamp& timestamp) -> std::chrono::nanoseconds;

[[nodiscard]] constexpr auto toString(const ::zenoh::WhatAmI& me) -> std::string_view {
  switch (me) {
    case ::zenoh::WhatAmI::Z_WHATAMI_ROUTER:
      return "Router";
    case ::zenoh::WhatAmI::Z_WHATAMI_PEER:
      return "Peer";
    case ::zenoh::WhatAmI::Z_WHATAMI_CLIENT:
      return "Client";
  }
}

[[nodiscard]] constexpr auto toMode(const ::zenoh::WhatAmI& me) -> Mode {
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

}  // namespace heph::ipc::zenoh
