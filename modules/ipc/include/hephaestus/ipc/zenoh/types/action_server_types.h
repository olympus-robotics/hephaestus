//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

namespace heph::ipc::zenoh {
enum class ActionServerRequestStatus : uint8_t {
  ACCEPTED = 0,
  REJECTED_USER = 1,
  REJECTED_ALREADY_RUNNING = 2,
  INVALID = 3,
};

struct ActionServerRequestResponse {
  ActionServerRequestStatus status;
};

}  // namespace heph::ipc::zenoh
