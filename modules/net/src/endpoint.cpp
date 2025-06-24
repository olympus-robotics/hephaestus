//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/net/endpoint.h"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <system_error>

#include <arpa/inet.h>
#include <fmt/format.h>
#include <hephaestus/utils/exception.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace heph::net {
Endpoint::Endpoint(IpFamily family) : Endpoint(family, "", 0) {
}

Endpoint::Endpoint(IpFamily family, const std::string& ip) : Endpoint(family, ip, 0) {
}

Endpoint::Endpoint(IpFamily family, const std::string& ip, std::uint16_t port)
  : address_(family == IpFamily::V4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) {
  if (family == IpFamily::V4) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0;
    if (!ip.empty()) {
      const int res = ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
      if (res == 0) {
        heph::panic("Invalid IPv4 address: {}", ip);
      }
      if (res == -1) {
        heph::panic("IPv4 is not supported");
      }
    }
    addr.sin_port = ::htons(port);
    std::memcpy(address_.data(), &addr, sizeof(addr));
  } else {
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = { { { 0 } } };
    if (!ip.empty()) {
      const int res = inet_pton(AF_INET6, ip.c_str(), &addr.sin6_addr);
      if (res == 0) {
        heph::panic("Invalid IPv6 address: {}", ip);
      }
      if (res == -1) {
        heph::panic("IPv6 is not supported");
      }
    }
    addr.sin6_port = ::htons(port);
    std::memcpy(address_.data(), &addr, sizeof(addr));
  }
}

auto Endpoint::family() const -> int {
  sockaddr addr{};
  std::memcpy(&addr, address_.data(), sizeof(addr));
  return addr.sa_family;
}

auto Endpoint::port() const -> std::uint16_t {
  std::uint16_t port{ 0 };
  if (family() == AF_INET) {
    sockaddr_in addr{};
    std::memcpy(&addr, address_.data(), sizeof(addr));
    port = addr.sin_port;
  } else {
    sockaddr_in6 addr{};
    std::memcpy(&addr, address_.data(), sizeof(addr));
    port = addr.sin6_port;
  }

  return ::ntohs(port);
}

auto Endpoint::ip() const -> std::string {
  std::array<char, INET6_ADDRSTRLEN> buffer{};

  auto convert = [&](void* addr) {
    if (::inet_ntop(family(), addr, buffer.data(), static_cast<socklen_t>(buffer.size())) == nullptr) {
      heph::panic("Could not convert sockaddr to ip: {}",
                  std::error_code(errno, std::system_category()).message());
    }
  };

  if (family() == AF_INET) {
    sockaddr_in addr{};
    std::memcpy(&addr, address_.data(), sizeof(addr));
    convert(&addr.sin_addr);
  } else {
    sockaddr_in6 addr{};
    std::memcpy(&addr, address_.data(), sizeof(addr));
    convert(&addr.sin6_addr);
  }

  return std::string{ buffer.data() };
}

auto Endpoint::nativeHandle() const -> std::span<const std::byte> {
  return { address_ };
}

auto Endpoint::nativeHandle() -> std::span<std::byte> {
  return { address_ };
}

auto format_as(const Endpoint& endpoint) -> std::string {  // NOLINT(readability-identifier-naming)
  return fmt::format("{}:{}", endpoint.ip(), endpoint.port());
}
}  // namespace heph::net
