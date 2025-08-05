//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <utility>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
class Acceptor {
public:
  static constexpr int DEFAULT_BACKLOG = 10;

  static auto createTcpIpV4(concurrency::Context& context) -> Acceptor;
  static auto createTcpIpV6(concurrency::Context& context) -> Acceptor;
#ifndef DISABLE_BLUETOOTH
  static auto createL2cap(concurrency::Context& context) -> Acceptor;
#endif

  void listen(int backlog = DEFAULT_BACKLOG) const;
  void bind(const Endpoint& endpoint) const;
  [[nodiscard]] auto localEndpoint() const -> Endpoint;
  [[nodiscard]] auto nativeHandle() const -> int;

  [[nodiscard]] auto type() const {
    return socket_.type();
  }

  [[nodiscard]] auto context() const -> concurrency::Context& {
    return socket_.context();
  }

private:
  explicit Acceptor(Socket socket) noexcept : socket_(std::move(socket)) {
  }

private:
  Socket socket_;
};
}  // namespace heph::net
