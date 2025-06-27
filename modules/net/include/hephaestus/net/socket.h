//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "hephaestus/net/endpoint.h"

namespace heph::net {

enum struct Protocol : std::uint8_t { TCP, UDP, BT };

class Socket {
public:
  Socket() = default;
  explicit Socket(int fd);
  Socket(IpFamily family, Protocol protocol);
  ~Socket() noexcept;

  Socket(const Socket&) = delete;
  auto operator=(const Socket&) -> Socket& = delete;
  Socket(Socket&& other) noexcept;
  auto operator=(Socket&& other) noexcept -> Socket&;

  void close() noexcept;

  void bind(const Endpoint& endpoint) const;

  void connect(const Endpoint& endpoint) const;

  [[nodiscard]] auto localEndpoint() const -> Endpoint;

  [[nodiscard]] auto remoteEndpoint() const -> Endpoint;

  [[nodiscard]] auto nativeHandle() const -> int;

  [[nodiscard]] auto maximumRecvSize() const {
    return maximum_recv_size_;
  }

  [[nodiscard]] auto maximumSendSize() const {
    return maximum_send_size_;
  }

private:
  void setupBTSocket(bool set_mtu);
  void setupUDPSocket();

private:
  std::size_t maximum_recv_size_{ std::numeric_limits<std::size_t>::max() };
  std::size_t maximum_send_size_{ std::numeric_limits<std::size_t>::max() };
  int fd_{ -1 };
};
}  // namespace heph::net
