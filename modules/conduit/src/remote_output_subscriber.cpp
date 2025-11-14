//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/remote_output_subscriber.h"

#include <cstddef>
#include <exception>
#include <span>
#include <string>
#include <vector>

#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log/log.h"

namespace heph::conduit::internal {
auto RemoteSubscriberOperator::trigger(heph::concurrency::Context* context, std::string* type_info)
    -> exec::task<MsgT> {
  try {
    if (!socket_.has_value()) {
      socket_.emplace(internal::createNetEntity<heph::net::Socket>(endpoint_, *context));

      auto error = co_await internal::connect(*socket_, endpoint_, *type_info, type_, name_);
      if (error != CONNECT_SUCCESS) {
        heph::panic("Could not connect: {}", error);
      }
    }

    auto msg = co_await (internal::recv<std::pmr::vector<std::byte>>(socket_.value(), &memory_resource_) |
                         stdexec::upon_stopped([] { return std::pmr::vector<std::byte>{}; }));
    if (type_.reliable) {
      std::byte ack{};
      co_await ((heph::net::sendAll(socket_.value(), std::span{ &ack, 1 }) |
                 stdexec::then([](std::span<const std::byte> /*unused*/) {})) |
                stdexec::upon_stopped([&] { msg.clear(); }));
    }
    if (!msg.empty()) {
      co_return msg;
    }
    heph::log(heph::ERROR, "Reconnecting subscriber, connection was closed", "node", name());

  } catch (std::exception& exception) {
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
  }

  socket_.reset();
  co_return {};
}
}  // namespace heph::conduit::internal
