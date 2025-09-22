//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <fmt/format.h>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::conduit {

namespace internal {
class RemoteSubscriberOperator {
public:
  using MsgT = std::pmr::vector<std::byte>;

  explicit RemoteSubscriberOperator(heph::net::Endpoint endpoint, std::string name, bool reliable = true)
    : endpoint_(std::move(endpoint))
    , name_(std::move(name))
    , type_{ .type = RemoteNodeType::OUTPUT, .reliable = reliable } {
  }

  [[nodiscard]] auto name() const -> std::string {
    return fmt::format("{}/{}", endpoint_, name_);
  }

  auto trigger(heph::concurrency::Context* context, std::string* type_info) -> exec::task<MsgT>;

private:
private:
  std::optional<heph::net::Socket> socket_;
  heph::net::Endpoint endpoint_;
  std::pmr::unsynchronized_pool_resource memory_resource_;
  std::string name_;
  std::string type_info_;
  std::optional<std::string> last_error_;
  RemoteNodeType type_;
};
}  // namespace internal

template <typename T>
struct RemoteOutputSubscriber
  : heph::conduit::Node<RemoteOutputSubscriber<T>, internal::RemoteSubscriberOperator> {
  std::string type_info = heph::serdes::getSerializedTypeInfo<T>().toJson();

  static auto name(const RemoteOutputSubscriber& self) {
    return self.data().name();
  }

  static auto trigger(RemoteOutputSubscriber* self) {
    return self->data().trigger(&self->engine().scheduler().context(), &self->type_info);
  }

  static auto execute(internal::RemoteSubscriberOperator::MsgT msg) -> std::optional<T> {
    auto msg_buffer = std::span{ msg };
    if (msg_buffer.empty()) {
      // No data received...
      return std::nullopt;
    }

    T value;
    heph::serdes::deserialize(msg_buffer, value);
    return value;
  }
};
}  // namespace heph::conduit
