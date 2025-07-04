//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/net/acceptor.h"

#include <cerrno>
#include <system_error>

#include <sys/socket.h>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/utils/exception.h"

namespace heph::net {
auto Acceptor::createTcpIpV4(concurrency::Context& context) -> Acceptor {
  return Acceptor{ Socket::createTcpIpV4(context) };
}
auto Acceptor::createTcpIpV6(concurrency::Context& context) -> Acceptor {
  return Acceptor{ Socket::createTcpIpV6(context) };
}
#ifndef DISABLE_BLUETOOTH
auto Acceptor::createL2cap(concurrency::Context& context) -> Acceptor {
  return Acceptor{ Socket::createL2cap(context) };
}
#endif

void Acceptor::listen(int backlog) const {
  const int res = ::listen(socket_.nativeHandle(), backlog);
  if (res == -1) {
    panic("listen: {}", std::error_code(errno, std::system_category()).message());
  }
}
void Acceptor::bind(const Endpoint& endpoint) const {
  socket_.bind(endpoint);
}
auto Acceptor::localEndpoint() const -> Endpoint {
  return socket_.localEndpoint();
}
auto Acceptor::nativeHandle() const -> int {
  return socket_.nativeHandle();
}
}  // namespace heph::net
