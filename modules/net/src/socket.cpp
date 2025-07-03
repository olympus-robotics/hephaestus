//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/net/socket.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <system_error>
#include <utility>
#include <vector>

#include <asm-generic/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/utils/exception.h"

namespace heph::net {

namespace {
auto familyToEndpointType(int domain) {
  switch (domain) {
    case AF_INET:
      return heph::net::EndpointType::IPV4;
    case AF_INET6:
      return heph::net::EndpointType::IPV6;
    case AF_BLUETOOTH:
      return heph::net::EndpointType::BT;
    default:
      heph::panic("Unknown domain {}", domain);
  }
  __builtin_unreachable();
}
}  // namespace

Socket::Socket(concurrency::Context* context, int fd, SocketType type)
  : context_(context), fd_(fd), type_(type) {
  if (fd_ == -1) {
    panic("socket: {}", std::error_code(errno, std::system_category()).message());
  }

  switch (type) {
    case SocketType::L2CAP:
      setupL2capSocket(true);
      break;
    case SocketType::UDP:
      setupUDPSocket();
      break;
    default:
      break;
  }
}

auto Socket::createTcpIpV4(concurrency::Context& context) -> Socket {
  return Socket{ &context, socket(AF_INET, SOCK_STREAM, 0), SocketType::TCP };
}
auto Socket::createTcpIpV6(concurrency::Context& context) -> Socket {
  return Socket{ &context, socket(AF_INET6, SOCK_STREAM, 0), SocketType::TCP };
}
auto Socket::createUdpIpV4(concurrency::Context& context) -> Socket {
  return Socket{ &context, socket(AF_INET, SOCK_DGRAM, 0), SocketType::UDP };
}
auto Socket::createUdpIpV6(concurrency::Context& context) -> Socket {
  return Socket{ &context, socket(AF_INET6, SOCK_DGRAM, 0), SocketType::UDP };
}
auto Socket::createL2cap(concurrency::Context& context) -> Socket {
  return Socket{ &context, socket(AF_BLUETOOTH, SOCK_SEQPACKET, 0), SocketType::L2CAP };
}

Socket::~Socket() noexcept {
  close();
}

Socket::Socket(Socket&& other) noexcept
  : context_(other.context_)
  , maximum_recv_size_(other.maximum_recv_size_)
  , maximum_send_size_(other.maximum_send_size_)
  , fd_(other.fd_)
  , type_(other.type_) {
  other.fd_ = -1;
}

auto Socket::operator=(Socket&& other) noexcept -> Socket& {
  close();
  context_ = other.context_;
  maximum_recv_size_ = other.maximum_recv_size_;
  maximum_send_size_ = other.maximum_send_size_;
  fd_ = other.fd_;
  type_ = other.type_;
  other.fd_ = -1;

  return *this;
}

void Socket::close() noexcept {
  if (fd_ != -1) {
    ::shutdown(fd_, SHUT_RDWR);
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

  int res = ::getsockname(fd_, &addr, &length);
  if (res == -1) {
    panic("getsockname: {}", std::error_code(errno, std::system_category()).message());
  }

  std::vector<std::byte> address(length);
  if (length <= sizeof(addr)) {
    std::memcpy(address.data(), &addr, length);
  } else {
    // NOLINTNEXTLINE
    res = ::getsockname(fd_, reinterpret_cast<sockaddr*>(address.data()), &length);
    if (res == -1) {
      panic("getsockname: {}", std::error_code(errno, std::system_category()).message());
    }
  }

  return Endpoint{ familyToEndpointType(addr.sa_family), std::move(address) };
}

auto Socket::remoteEndpoint() const -> Endpoint {
  sockaddr addr{};
  socklen_t length{ sizeof(addr) };

  int res = ::getpeername(fd_, &addr, &length);
  if (res == -1) {
    panic("getpeername: {}", std::error_code(errno, std::system_category()).message());
  }

  std::vector<std::byte> address(length);
  if (length <= sizeof(addr)) {
    std::memcpy(address.data(), &addr, length);
  } else {
    // NOLINTNEXTLINE
    res = ::getpeername(fd_, reinterpret_cast<sockaddr*>(address.data()), &length);
    if (res == -1) {
      panic("getsockname: {}", std::error_code(errno, std::system_category()).message());
    }
  }

  return Endpoint{ familyToEndpointType(addr.sa_family), std::move(address) };
}

void Socket::setupL2capSocket(bool set_mtu) {
  static constexpr std::uint16_t BT_TX_WIN_SIZE = 256;
  static constexpr std::uint16_t BT_MAX_TX = 100;
  static constexpr std::size_t BT_PACKET_SIZE = 65535;
  if (set_mtu) {
    struct l2cap_options opts{};
    socklen_t optlen = sizeof(opts);
    if (getsockopt(fd_, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen) < 0) {
      panic("unable to get l2cap options: {}", std::error_code(errno, std::system_category()).message());
    }

    opts.imtu = BT_PACKET_SIZE;
    opts.omtu = BT_PACKET_SIZE;
    opts.mode = L2CAP_MODE_ERTM;
    opts.fcs = 1;
    opts.flush_to = 0;
    opts.txwin_size = BT_TX_WIN_SIZE;
    opts.max_tx = BT_MAX_TX;

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
  static constexpr std::size_t MAX_UDP_PACKET_SIZE = 65507;
  maximum_recv_size_ = MAX_UDP_PACKET_SIZE;
  maximum_send_size_ = MAX_UDP_PACKET_SIZE;
}
}  // namespace heph::net
