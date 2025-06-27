//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/net/endpoint.h"

namespace heph::net {
namespace internal {
template <typename>
struct AcceptOperation;
}

enum struct SocketType : std::uint8_t { TCP, UDP, L2CAP, INVALID };

class Socket {
public:
  Socket() = default;
  ~Socket() noexcept;

  Socket(const Socket&) = delete;
  auto operator=(const Socket&) -> Socket& = delete;
  Socket(Socket&& other) noexcept;
  auto operator=(Socket&& other) noexcept -> Socket&;

  static auto createTcpIpV4(concurrency::Context& context) -> Socket;
  static auto createTcpIpV6(concurrency::Context& context) -> Socket;
  static auto createUdpIpV4(concurrency::Context& context) -> Socket;
  static auto createUdpIpV6(concurrency::Context& context) -> Socket;
  static auto createL2cap(concurrency::Context& context) -> Socket;

  [[nodiscard]] auto type() const {
    return type_;
  }

  void close() noexcept;

  void bind(const Endpoint& endpoint) const;

  void connect(const Endpoint& endpoint) const;

  [[nodiscard]] auto localEndpoint() const -> Endpoint;

  [[nodiscard]] auto remoteEndpoint() const -> Endpoint;

  [[nodiscard]] auto nativeHandle() const -> int {
    return fd_;
  }

  [[nodiscard]] auto context() const -> concurrency::Context& {
    return *context_;
  }

  [[nodiscard]] auto maximumRecvSize() const {
    return maximum_recv_size_;
  }

  [[nodiscard]] auto maximumSendSize() const {
    return maximum_send_size_;
  }

private:
  Socket(concurrency::Context* context, int fd, SocketType type);
  void setupL2capSocket(bool set_mtu);
  void setupUDPSocket();

  template <typename>
  friend struct internal::AcceptOperation;

private:
  concurrency::Context* context_{ nullptr };
  std::size_t maximum_recv_size_{ std::numeric_limits<std::size_t>::max() };
  std::size_t maximum_send_size_{ std::numeric_limits<std::size_t>::max() };
  int fd_{ -1 };
  SocketType type_{ SocketType::INVALID };
};
}  // namespace heph::net
