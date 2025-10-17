//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#if 0
#include "hephaestus/conduit/remote_input_publisher.h"

#include <cstddef>
#include <exception>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log/log.h"

namespace heph::conduit::internal {
auto SetRemoteInputOperator::execute(std::vector<std::byte> msg, std::string* type_info) -> exec::task<bool> {
  try {
    if (!socket_.has_value()) {
      socket_.emplace(internal::createNetEntity<heph::net::Socket>(endpoint_, *context_));

      auto error = co_await internal::connect(*socket_, endpoint_, *type_info, type_, name_);
      if (error != CONNECT_SUCCESS) {
        heph::panic("Could not connect: {}", error);
      }
    }

    co_await (internal::sendMsg(socket_.value(), name(), std::move(msg)) |
              stdexec::upon_stopped([this] { socket_.reset(); }));
    if (type_.reliable) {
      std::byte ack{};
      co_await ((heph::net::recvAll(socket_.value(), std::span<std::byte>{ &ack, 1 }) |
                 stdexec::then([](std::span<std::byte> /*unused*/) {})) |
                stdexec::upon_stopped([this] { socket_.reset(); }));
    }

  } catch (std::exception& exception) {
    socket_.reset();
    std::string error = exception.what();
    if (last_error_.has_value()) {
      if (*last_error_ == error) {
        error.clear();
      } else {
        last_error_.emplace(error);
      }
    } else {
      last_error_.emplace(error);
    }

    if (!error.empty()) {
      heph::log(heph::ERROR, "Retrying", "node", name(), "error", error);
    }
    co_return false;
  }
  co_return true;
}

}  // namespace heph::conduit::internal
#endif
