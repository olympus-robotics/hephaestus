//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

namespace heph::ipc::zenoh {
enum class ActionServerRequestStatus : uint8_t {
  SUCCESSFUL = 0,
  REJECTED_USER = 1,
  REJECTED_ALREADY_RUNNING = 2,
  INVALID = 3,
  STOPPED = 4,
};

struct ActionServerRequestResponse {
  ActionServerRequestStatus status;
};

template <typename ReplyT>
struct ActionServerResponse {
  ReplyT value;
  ActionServerRequestStatus status{};
};

}  // namespace heph::ipc::zenoh
