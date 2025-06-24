//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/net/socket.h"

#include <cerrno>
#include <cstring>
#include <system_error>

#include <fmt/format.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hephaestus/net/endpoint.h"
#include "hephaestus/utils/exception.h"

namespace heph::net {

Socket::Socket(int fd) : fd_(fd) {
}

Socket::Socket(IpFamily family, Protocol protocol)
  : fd_(::socket(family == IpFamily::V4 ? AF_INET : AF_INET6,
                 protocol == Protocol::TCP ? SOCK_STREAM : SOCK_DGRAM, 0)) {
  if (fd_ == -1) {
    heph::panic("socket: {}", std::error_code(errno, std::system_category()).message());
  }
}

Socket::~Socket() noexcept {
  close();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
  other.fd_ = -1;
}

auto Socket::operator=(Socket&& other) noexcept -> Socket& {
  close();
  fd_ = other.fd_;
  other.fd_ = -1;

  return *this;
}

void Socket::close() noexcept {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

void Socket::bind(const Endpoint& endpoint) const {
  auto handle = endpoint.nativeHandle();
  const int res =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      ::bind(fd_, reinterpret_cast<const sockaddr*>(handle.data()), static_cast<socklen_t>(handle.size()));
  if (res == -1) {
    heph::panic("bind: {}", std::error_code(errno, std::system_category()).message());
  }
}

void Socket::connect(const Endpoint& endpoint) const {
  auto handle = endpoint.nativeHandle();
  const int res =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      ::connect(fd_, reinterpret_cast<const sockaddr*>(handle.data()), static_cast<socklen_t>(handle.size()));
  if (res == -1) {
    heph::panic("connect: {}", std::error_code(errno, std::system_category()).message());
  }
}

auto Socket::localEndpoint() const -> Endpoint {
  sockaddr addr{};
  socklen_t length{ sizeof(addr) };

  const int res = ::getsockname(fd_, &addr, &length);
  if (res == -1) {
    heph::panic("getsockname: {}", std::error_code(errno, std::system_category()).message());
  }

  Endpoint endpoint(addr.sa_family == AF_INET ? IpFamily::V4 : IpFamily::V6);
  auto handle = endpoint.nativeHandle();
  if (length != handle.size()) {
    heph::panic("mismatch of sockaddr length got {}, expected {}", length, handle.size());
  }

  std::memcpy(handle.data(), &addr, handle.size());

  return endpoint;
}

auto Socket::remoteEndpoint() const -> Endpoint {
  sockaddr addr{};
  socklen_t length{ sizeof(addr) };

  const int res = ::getpeername(fd_, &addr, &length);
  if (res == -1) {
    heph::panic("getpeername: {}", std::error_code(errno, std::system_category()).message());
  }

  Endpoint endpoint(addr.sa_family == AF_INET ? IpFamily::V4 : IpFamily::V6);
  auto handle = endpoint.nativeHandle();
  if (length != handle.size()) {
    heph::panic("mismatch of sockaddr length got {}, expected {}", length, handle.size());
  }

  std::memcpy(handle.data(), &addr, handle.size());

  return endpoint;
}

auto Socket::nativeHandle() const -> int {
  return fd_;
}
}  // namespace heph::net
