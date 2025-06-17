//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace heph::net {

enum struct IpFamily : std::uint8_t { V4, V6 };

class Endpoint {
public:
  Endpoint() = default;
  ~Endpoint() = default;
  explicit Endpoint(IpFamily family);
  Endpoint(IpFamily family, const std::string& ip);
  Endpoint(IpFamily family, const std::string& ip, std::uint16_t port);

  Endpoint(const Endpoint&) = default;
  Endpoint(Endpoint&&) = default;
  auto operator=(const Endpoint&) -> Endpoint& = default;
  auto operator=(Endpoint&&) -> Endpoint& = default;

  friend auto operator==(const Endpoint& lhs, const Endpoint& rhs) -> bool = default;

  [[nodiscard]] auto nativeHandle() const -> std::span<const std::byte>;
  [[nodiscard]] auto nativeHandle() -> std::span<std::byte>;

  [[nodiscard]] auto family() const -> int;
  [[nodiscard]] auto ip() const -> std::string;
  [[nodiscard]] auto port() const -> std::uint16_t;

private:
  std::vector<std::byte> address_;
};

auto format_as(const Endpoint& endpoint) -> std::string;  // NOLINT(readability-identifier-naming)
}  // namespace heph::net
