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
#include <vector>

#include <arpa/inet.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <fmt/format.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "hephaestus/utils/exception.h"

namespace heph::net {
Endpoint::Endpoint(IpFamily family) : Endpoint(family, "", 0) {
}

Endpoint::Endpoint(IpFamily family, const std::string& ip) : Endpoint(family, ip, 0) {
}

namespace {
auto createIpV4(const std::string& ip, std::uint16_t port) -> std::vector<std::byte> {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = 0;
  if (!ip.empty()) {
    const int res = ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (res == 0) {
      panic("Invalid IPv4 address: {}", ip);
    }
    if (res == -1) {
      panic("IPv4 is not supported");
    }
  }
  addr.sin_port = ::htons(port);
  std::vector<std::byte> address(sizeof(addr));
  std::memcpy(address.data(), &addr, sizeof(addr));
  return address;
}
auto createIpV6(const std::string& ip, std::uint16_t port) -> std::vector<std::byte> {
  sockaddr_in6 addr{};
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = { { { 0 } } };
  if (!ip.empty()) {
    const int res = inet_pton(AF_INET6, ip.c_str(), &addr.sin6_addr);
    if (res == 0) {
      panic("Invalid IPv6 address: {}", ip);
    }
    if (res == -1) {
      panic("IPv6 is not supported");
    }
  }
  addr.sin6_port = ::htons(port);
  std::vector<std::byte> address(sizeof(addr));
  std::memcpy(address.data(), &addr, sizeof(addr));
  return address;
}
auto createBt(const std::string& ip, std::uint16_t port) -> std::vector<std::byte> {
  sockaddr_l2 addr{};
  addr.l2_family = AF_BLUETOOTH;

  addr.l2_bdaddr = {};
  if (!ip.empty()) {
    const int res = str2ba(ip.c_str(), &addr.l2_bdaddr);
    if (res != 0) {
      panic("Invalid BT address {}", ip);
    }
  }
  addr.l2_psm = htobs(port);
  std::vector<std::byte> address(sizeof(addr));
  std::memcpy(address.data(), &addr, sizeof(addr));
  return address;
}

auto createAddress(IpFamily family, const std::string& ip, std::uint16_t port) -> std::vector<std::byte> {
  switch (family) {
    case IpFamily::V4:
      return createIpV4(ip, port);
    case IpFamily::V6:
      return createIpV6(ip, port);
    case heph::net::IpFamily::BT:
      return createBt(ip, port);
  }
}
}  // namespace

Endpoint::Endpoint(IpFamily family, const std::string& ip, std::uint16_t port)
  : address_(createAddress(family, ip, port)) {
}

auto Endpoint::family() const -> int {
  sockaddr addr{};
  std::memcpy(&addr, address_.data(), sizeof(addr));
  return addr.sa_family;
}

namespace {
auto getPortIpV4(const std::vector<std::byte>& address) -> std::uint16_t {
  sockaddr_in addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  return ::ntohs(addr.sin_port);
}
auto getPortIpV6(const std::vector<std::byte>& address) -> std::uint16_t {
  sockaddr_in6 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  return ::ntohs(addr.sin6_port);
}
auto getPortBt(const std::vector<std::byte>& address) -> std::uint16_t {
  sockaddr_l2 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  return btohs(addr.l2_psm);
}
}  // namespace

auto Endpoint::port() const -> std::uint16_t {
  switch (family()) {
    case AF_INET:
      return getPortIpV4(address_);
    case AF_INET6:
      return getPortIpV6(address_);
    case AF_BLUETOOTH:
      return getPortBt(address_);
    default:
      heph::panic("Unknown family");
  }
  __builtin_unreachable();
}
namespace {
auto getIpV4(const std::vector<std::byte>& address) -> std::string {
  std::array<char, INET_ADDRSTRLEN> buffer{};

  sockaddr_in addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  if (::inet_ntop(AF_INET, &addr.sin_addr, buffer.data(), static_cast<socklen_t>(buffer.size())) == nullptr) {
    panic("Could not convert sockaddr to ip: {}", std::error_code(errno, std::system_category()).message());
  }

  return std::string{ buffer.data() };
}
auto getIpV6(const std::vector<std::byte>& address) -> std::string {
  std::array<char, INET_ADDRSTRLEN> buffer{};

  sockaddr_in6 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  if (::inet_ntop(AF_INET6, &addr.sin6_addr, buffer.data(), static_cast<socklen_t>(buffer.size())) ==
      nullptr) {
    panic("Could not convert sockaddr to ip: {}", std::error_code(errno, std::system_category()).message());
  }

  return std::string{ buffer.data() };
}
auto getIpBt(const std::vector<std::byte>& address) -> std::string {
  static constexpr std::size_t BT_ADDRSTRLEN = 18;
  std::array<char, BT_ADDRSTRLEN> buffer{};

  sockaddr_l2 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));

  if (ba2str(&addr.l2_bdaddr, buffer.data()) == 0) {
    panic("Could not convert sockaddr to ip: {}", std::error_code(errno, std::system_category()).message());
  }

  return std::string{ buffer.data() };
}
}  // namespace

auto Endpoint::ip() const -> std::string {
  switch (family()) {
    case AF_INET:
      return getIpV4(address_);
    case AF_INET6:
      return getIpV6(address_);
    case AF_BLUETOOTH:
      return getIpBt(address_);
    default:
      heph::panic("Unknown family");
  }
  __builtin_unreachable();
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
