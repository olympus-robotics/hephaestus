//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
class Acceptor {
public:
  static constexpr int DEFAULT_BACKLOG = 10;

  Acceptor(IpFamily family, Protocol protocol);

  void listen(int backlog = DEFAULT_BACKLOG) const;
  void bind(const Endpoint& endpoint) const;
  [[nodiscard]] auto localEndpoint() const -> Endpoint;
  [[nodiscard]] auto nativeHandle() const -> int;

private:
  Socket socket_;
};
}  // namespace heph::net
