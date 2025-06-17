//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

#include "hephaestus/net/endpoint.h"

namespace heph::net {

enum struct Protocol : std::uint8_t { TCP, UDP };

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

private:
  int fd_{ -1 };
};
}  // namespace heph::net
