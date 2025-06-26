//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/net/socket.h"

#include <cerrno>
#include <cstring>
#include <system_error>

#include <asm-generic/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hephaestus/net/endpoint.h"
#include "hephaestus/utils/exception.h"

namespace heph::net {

void Socket::setupBTSocket(bool set_mtu) {
  if (localEndpoint().family() != AF_BLUETOOTH) {
    return;
  }
  static constexpr std::size_t BT_PACKET_SIZE = 65535;
  if (set_mtu) {
    struct l2cap_options opts{};
    socklen_t optlen = sizeof(opts);
    if (getsockopt(fd_, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen) < 0) {
      panic("unable to get l2cap options: {}", std::error_code(errno, std::system_category()).message());
    }

    opts.imtu = BT_PACKET_SIZE;
    opts.omtu = BT_PACKET_SIZE;
    // opts.mode = L2CAP_MODE_STREAMING;

    if (setsockopt(fd_, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen) < 0) {
      panic("unable to set l2cap options: {}", std::error_code(errno, std::system_category()).message());
    }
  }
  maximum_recv_size_ = BT_PACKET_SIZE;
  maximum_send_size_ = BT_PACKET_SIZE;

  static constexpr int KB = 1024;
  int bufsize = 4 * KB * KB;
  if (setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
    panic("unable to set receive buffer size: {}", std::error_code(errno, std::system_category()).message());
  }
  if (setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0) {
    panic("unable to set send buffer size: {}", std::error_code(errno, std::system_category()).message());
  }
}
void Socket::setupUDPSocket() {
  if (localEndpoint().family() == AF_BLUETOOTH) {
    return;
  }
  static constexpr std::size_t MAX_UDP_PACKET_SIZE = 65507;
  maximum_recv_size_ = MAX_UDP_PACKET_SIZE;
  maximum_send_size_ = MAX_UDP_PACKET_SIZE;
}

Socket::Socket(int fd) : fd_(fd) {
  setupBTSocket(false);
  setupUDPSocket();
}

namespace {
auto familyToDomain(IpFamily family) {
  switch (family) {
    case heph::net::IpFamily::V4:
      return AF_INET;
    case heph::net::IpFamily::V6:
      return AF_INET6;
    case heph::net::IpFamily::BT:
      return AF_BLUETOOTH;
  }
}
auto domainToFamily(int domain) {
  switch (domain) {
    case AF_INET:
      return heph::net::IpFamily::V4;
    case AF_INET6:
      return heph::net::IpFamily::V6;
    case AF_BLUETOOTH:
      return heph::net::IpFamily::BT;
    default:
      heph::panic("Unknown domain {}", domain);
  }
  __builtin_unreachable();
}
auto convertType(Protocol protocol) {
  switch (protocol) {
    case heph::net::Protocol::TCP:
      return SOCK_STREAM;
    case heph::net::Protocol::UDP:
      return SOCK_DGRAM;
    case heph::net::Protocol::BT:
      return SOCK_SEQPACKET;
  }
}
}  // namespace

Socket::Socket(IpFamily family, Protocol protocol)
  : fd_(::socket(familyToDomain(family), convertType(protocol), 0)) {
  if (fd_ == -1) {
    panic("socket: {}", std::error_code(errno, std::system_category()).message());
  }

  if (protocol == Protocol::BT) {
    int reuse = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
      panic("unable to set socket to reuse address: {}",
            std::error_code(errno, std::system_category()).message());
    }
    setupBTSocket(true);
  }
  if (protocol == Protocol::UDP) {
    setupUDPSocket();
  }
}

Socket::~Socket() noexcept {
  close();
}

Socket::Socket(Socket&& other) noexcept
  : maximum_recv_size_(other.maximum_recv_size_)
  , maximum_send_size_(other.maximum_send_size_)
  , fd_(other.fd_) {
  other.fd_ = -1;
}

auto Socket::operator=(Socket&& other) noexcept -> Socket& {
  close();
  maximum_recv_size_ = other.maximum_recv_size_;
  maximum_send_size_ = other.maximum_send_size_;
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
    panic("bind: {}", std::error_code(errno, std::system_category()).message());
  }
}

void Socket::connect(const Endpoint& endpoint) const {
  auto handle = endpoint.nativeHandle();
  const int res =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      ::connect(fd_, reinterpret_cast<const sockaddr*>(handle.data()), static_cast<socklen_t>(handle.size()));
  if (res == -1) {
    panic("connect: {}", std::error_code(errno, std::system_category()).message());
  }
}

auto Socket::localEndpoint() const -> Endpoint {
  sockaddr addr{};
  socklen_t length{ sizeof(addr) };

  const int res = ::getsockname(fd_, &addr, &length);
  if (res == -1) {
    panic("getsockname: {}", std::error_code(errno, std::system_category()).message());
  }

  Endpoint endpoint(domainToFamily(addr.sa_family));
  auto handle = endpoint.nativeHandle();
  if (length != handle.size()) {
    panic("mismatch of sockaddr length got {}, expected {}", length, handle.size());
  }

  std::memcpy(handle.data(), &addr, handle.size());

  return endpoint;
}

auto Socket::remoteEndpoint() const -> Endpoint {
  sockaddr addr{};
  socklen_t length{ sizeof(addr) };

  const int res = ::getpeername(fd_, &addr, &length);
  if (res == -1) {
    panic("getpeername: {}", std::error_code(errno, std::system_category()).message());
  }

  Endpoint endpoint(domainToFamily(addr.sa_family));
  auto handle = endpoint.nativeHandle();
  if (length != handle.size()) {
    panic("mismatch of sockaddr length got {}, expected {}", length, handle.size());
  }

  std::memcpy(handle.data(), &addr, handle.size());

  return endpoint;
}

auto Socket::nativeHandle() const -> int {
  return fd_;
}
}  // namespace heph::net
