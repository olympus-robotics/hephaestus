//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

namespace heph::ipc::zenoh::action_server {

enum class RequestStatus : uint8_t {
  SUCCESSFUL = 0,
  REJECTED_USER = 1,
  REJECTED_ALREADY_RUNNING = 2,
  INVALID = 3,
  STOPPED = 4,
};

struct RequestResponse {
  RequestStatus status;
};

template <typename ReplyT>
struct Response {
  ReplyT value;
  RequestStatus status{};
};

}  // namespace heph::ipc::zenoh::action_server
