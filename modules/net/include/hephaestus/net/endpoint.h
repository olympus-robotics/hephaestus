//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <linux/can.h>
#include <net/if.h>

namespace heph::net {

#ifndef DISABLE_BLUETOOTH
enum struct EndpointType : std::uint8_t { IPV4, IPV6, BT, SOCKETCAN, INVALID };
#else
enum struct EndpointType : std::uint8_t { IPV4, IPV6, INVALID };
#endif

struct SocketcanAddress {
  sockaddr_can addr;
  ifreq ifr;
};

class Endpoint {
public:
  Endpoint() = default;
  ~Endpoint() = default;
  Endpoint(const Endpoint&) = default;
  Endpoint(Endpoint&&) = default;
  auto operator=(const Endpoint&) -> Endpoint& = default;
  auto operator=(Endpoint&&) -> Endpoint& = default;

  friend auto operator==(const Endpoint& lhs, const Endpoint& rhs) -> bool = default;

  static auto createIpV4(const std::string& ip = "", std::uint16_t port = 0) -> Endpoint;

  static auto createIpV6(const std::string& ip = "", std::uint16_t port = 0) -> Endpoint;

#ifndef DISABLE_BLUETOOTH
  static auto createBt(const std::string& mac = "", std::uint16_t psm = 0) -> Endpoint;
#endif

  [[nodiscard]] static auto createSocketcan(const std::string& interface) -> Endpoint;

  [[nodiscard]] auto nativeHandle() const -> std::span<const std::byte>;
  [[nodiscard]] auto nativeHandle() -> std::span<std::byte>;

  [[nodiscard]] auto type() const {
    return type_;
  }
  [[nodiscard]] auto address() const -> std::string;
  [[nodiscard]] auto port() const -> std::uint16_t;

private:
  Endpoint(EndpointType type, std::vector<std::byte> address) noexcept
    : address_(std::move(address)), type_(type) {
  }

  friend class Socket;

private:
  std::vector<std::byte> address_;
  EndpointType type_{ EndpointType::INVALID };
};

auto format_as(const Endpoint& endpoint) -> std::string;  // NOLINT(readability-identifier-naming)
}  // namespace heph::net
