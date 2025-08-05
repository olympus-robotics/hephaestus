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
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#ifndef DISABLE_BLUETOOTH
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#endif
#include <fmt/format.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "hephaestus/utils/exception.h"

namespace heph::net {
auto Endpoint::createIpV4(const std::string& ip, std::uint16_t port) -> Endpoint {
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
  return Endpoint{ EndpointType::IPV4, std::move(address) };
}

auto Endpoint::createIpV6(const std::string& ip, std::uint16_t port) -> Endpoint {
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
  return Endpoint{ EndpointType::IPV6, std::move(address) };
}

#ifndef DISABLE_BLUETOOTH
auto Endpoint::createBt(const std::string& mac, std::uint16_t psm) -> Endpoint {
  sockaddr_l2 addr{};
  addr.l2_family = AF_BLUETOOTH;

  addr.l2_bdaddr = {};
  if (!mac.empty()) {
    const int res = str2ba(mac.c_str(), &addr.l2_bdaddr);
    if (res != 0) {
      panic("Invalid BT address {}", mac);
    }
  }
  addr.l2_psm = htobs(psm);
  std::vector<std::byte> address(sizeof(addr));
  std::memcpy(address.data(), &addr, sizeof(addr));
  return Endpoint{ EndpointType::BT, std::move(address) };
}
#endif

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
#ifndef DISABLE_BLUETOOTH
auto getPortBt(const std::vector<std::byte>& address) -> std::uint16_t {
  sockaddr_l2 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  return btohs(addr.l2_psm);
}
#endif
}  // namespace

auto Endpoint::port() const -> std::uint16_t {
  switch (type()) {
    case heph::net::EndpointType::IPV4:
      return getPortIpV4(address_);
    case heph::net::EndpointType::IPV6:
      return getPortIpV6(address_);
#ifndef DISABLE_BLUETOOTH
    case heph::net::EndpointType::BT:
      return getPortBt(address_);
#endif
    default:
      heph::panic("Unknown family");
  }
  __builtin_unreachable();
}

namespace {
auto getAddressV4(const std::vector<std::byte>& address) -> std::string {
  std::array<char, INET_ADDRSTRLEN> buffer{};

  sockaddr_in addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  if (::inet_ntop(AF_INET, &addr.sin_addr, buffer.data(), static_cast<socklen_t>(buffer.size())) == nullptr) {
    panic("Could not convert sockaddr to ip: {}", std::error_code(errno, std::system_category()).message());
  }

  return std::string{ buffer.data() };
}
auto getAddressIpV6(const std::vector<std::byte>& address) -> std::string {
  std::array<char, INET_ADDRSTRLEN> buffer{};

  sockaddr_in6 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));
  if (::inet_ntop(AF_INET6, &addr.sin6_addr, buffer.data(), static_cast<socklen_t>(buffer.size())) ==
      nullptr) {
    panic("Could not convert sockaddr to ip: {}", std::error_code(errno, std::system_category()).message());
  }

  return std::string{ buffer.data() };
}
#ifndef DISABLE_BLUETOOTH
auto getAddressBt(const std::vector<std::byte>& address) -> std::string {
  static constexpr std::size_t BT_ADDRSTRLEN = 18;
  std::array<char, BT_ADDRSTRLEN> buffer{};

  sockaddr_l2 addr{};
  std::memcpy(&addr, address.data(), sizeof(addr));

  if (ba2str(&addr.l2_bdaddr, buffer.data()) == 0) {
    panic("Could not convert sockaddr to ip: {}", std::error_code(errno, std::system_category()).message());
  }

  return std::string{ buffer.data() };
}
#endif
}  // namespace

auto Endpoint::address() const -> std::string {
  switch (type()) {
    case heph::net::EndpointType::IPV4:
      return getAddressV4(address_);
    case heph::net::EndpointType::IPV6:
      return getAddressIpV6(address_);
#ifndef DISABLE_BLUETOOTH
    case heph::net::EndpointType::BT:
      return getAddressBt(address_);
#endif
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
  return fmt::format("{}:{}", endpoint.address(), endpoint.port());
}
}  // namespace heph::net
