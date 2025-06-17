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
  Endpoint(IpFamily family, std::string const& ip);
  Endpoint(IpFamily family, std::string const& ip, std::uint16_t port);

  Endpoint(Endpoint const&) = default;
  Endpoint(Endpoint&&) = default;
  auto operator=(Endpoint const&) -> Endpoint& = default;
  auto operator=(Endpoint&&) -> Endpoint& = default;

  friend auto operator==(Endpoint const& lhs, Endpoint const& rhs) -> bool = default;

  [[nodiscard]] auto nativeHandle() const -> std::span<std::byte const>;
  [[nodiscard]] auto nativeHandle() -> std::span<std::byte>;

  [[nodiscard]] auto family() const -> int;
  [[nodiscard]] auto ip() const -> std::string;
  [[nodiscard]] auto port() const -> std::uint16_t;

private:
  std::vector<std::byte> address_;
};

auto format_as(Endpoint const& endpoint) -> std::string;  // NOLINT(readability-identifier-naming)
}  // namespace heph::net
