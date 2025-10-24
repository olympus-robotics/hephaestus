//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/net/connect.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"

namespace heph::conduit::internal {
template <typename Container, typename... Ts>
auto recv(heph::net::Socket& socket, Ts&&... ts) {
  auto message = Container{ std::forward<Ts>(ts)... };
  return stdexec::just(static_cast<std::uint16_t>(0)) |
         stdexec::let_value([&socket, message = std::move(message)](auto& size) {
           return heph::net::recvAll(socket, std::as_writable_bytes(std::span{ &size, 1 })) |
                  stdexec::let_value([&socket, &size, message = std::move(message)](auto /*unused*/) mutable {
                    message.resize(size);
                    return heph::net::recvAll(socket, std::as_writable_bytes(std::span{ message })) |
                           stdexec::then([&message](auto /*unused*/) { return std::move(message); });
                  });
         });
}
template <typename Container>
auto send(heph::net::Socket& socket, const Container& message) {
  if (message.size() > std::numeric_limits<std::uint16_t>::max()) {
    heph::panic("message too big ({})", message.size());
  }

  return stdexec::just(static_cast<std::uint16_t>(message.size())) |
         stdexec::let_value([&socket, &message](const std::uint16_t& size) {
           return heph::net::sendAll(socket, std::as_bytes(std::span{ &size, 1 })) |
                  stdexec::let_value([&socket, &message](auto /*unused*/) {
                    return heph::net::sendAll(socket, std::as_bytes(std::span{ message })) |
                           stdexec::then([](auto /*unused*/) {});
                  });
         });
}
template <typename T>
auto createNetEntity(heph::concurrency::Context& context, const heph::net::Endpoint& endpoint) {
  switch (endpoint.type()) {
#ifndef DISABLE_BLUETOOTH
    case heph::net::EndpointType::BT:
      return T::createL2cap(context);
#endif
    case heph::net::EndpointType::IPV4:
      return T::createTcpIpV4(context);
    case heph::net::EndpointType::IPV6:
      return T::createTcpIpV6(context);
    default:
      heph::panic("Unknown endpoint type");
  }
  __builtin_unreachable();
}
inline auto connect(heph::net::Socket& socket, const heph::net::Endpoint& endpoint,
                    const std::string& type_info, std::uint64_t& type, const std::string& name) {
  return net::connect(socket, endpoint) | stdexec::let_value([&] {
           return net::sendAll(socket, std::as_bytes(std::span{ &type, 1 })) |
                  stdexec::let_value([&](std::span<const std::byte> /*recv_buffer*/) {
                    return internal::send(socket, name) |
                           stdexec::let_value([&] { return internal::send(socket, type_info); });
                  }) |
                  stdexec::let_value([&socket] { return internal::recv<std::string>(socket); });
         });
}
}  // namespace heph::conduit::internal
