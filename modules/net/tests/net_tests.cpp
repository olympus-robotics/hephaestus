//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <cstring>
#include <numeric>
#include <span>
#include <system_error>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"

namespace heph::net {

TEST(Net, Ipv4Endpoint) {
  const Endpoint ep0;
  auto ep1(Endpoint::createIpV4());
  const auto ep2(Endpoint::createIpV4());
  auto ep3(Endpoint::createIpV4("127.0.0.1"));
  const auto ep4(Endpoint::createIpV4("127.0.0.1"));
  const auto ep5(Endpoint::createIpV4("127.0.0.1", 1));
  const auto ep6(Endpoint::createIpV4("127.0.0.1", 1));
  const auto ep7(Endpoint::createIpV6());

  EXPECT_NE(ep0, ep1);
  EXPECT_NE(ep7, ep4);
  EXPECT_EQ(ep1, ep2);
  EXPECT_EQ(ep3, ep4);
  EXPECT_EQ(ep5, ep6);
  EXPECT_NE(ep1, ep3);
  EXPECT_NE(ep1, ep5);
  EXPECT_NE(ep3, ep5);

  EXPECT_EQ(fmt::format("{}", ep1), "0.0.0.0:0");
  EXPECT_EQ(fmt::format("{}", ep3), "127.0.0.1:0");
  EXPECT_EQ(fmt::format("{}", ep6), "127.0.0.1:1");

  ep3 = ep6;
  EXPECT_EQ(ep3, ep6);
  EXPECT_NE(ep3, ep4);

  auto handle1 = ep1.nativeHandle();
  auto handle2 = ep3.nativeHandle();
  std::memcpy(handle1.data(), handle2.data(), handle2.size());
  EXPECT_EQ(ep3, ep1);

  EXPECT_DEATH(Endpoint::createIpV4("."), "");
}

TEST(Net, Ipv6Endpoint) {
  const Endpoint ep0;
  auto ep1(Endpoint::createIpV6());
  const auto ep2(Endpoint::createIpV6());
  auto ep3(Endpoint::createIpV6("0:0:0:0:0:FFFF:204.152.189.116"));
  const auto ep4(Endpoint::createIpV6("0:0:0:0:0:FFFF:204.152.189.116"));
  const auto ep5(Endpoint::createIpV6("0:0:0:0:0:FFFF:204.152.189.116", 1));
  const auto ep6(Endpoint::createIpV6("0:0:0:0:0:FFFF:204.152.189.116", 1));
  const auto ep7(Endpoint::createIpV4());

  EXPECT_NE(ep0, ep1);
  EXPECT_EQ(ep1, ep2);
  EXPECT_EQ(ep3, ep4);
  EXPECT_EQ(ep5, ep6);
  EXPECT_NE(ep1, ep3);
  EXPECT_NE(ep1, ep5);
  EXPECT_NE(ep3, ep5);
  EXPECT_NE(ep7, ep4);

  ep3 = ep6;
  EXPECT_EQ(ep3, ep6);
  EXPECT_NE(ep3, ep4);

  auto handle1 = ep1.nativeHandle();
  auto handle2 = ep3.nativeHandle();
  std::memcpy(handle1.data(), handle2.data(), handle2.size());
  EXPECT_EQ(ep3, ep1);

  EXPECT_DEATH(Endpoint::createIpV6(":"), "");
}

TEST(Net, BtEndpoint) {
  const Endpoint ep0;
  auto ep1(Endpoint::createBt());
  const auto ep2(Endpoint::createBt());
  auto ep3(Endpoint::createBt("01:02:03:04:05:07"));
  const auto ep4(Endpoint::createBt("01:02:03:04:05:07"));
  const auto ep5(Endpoint::createBt("01:02:03:04:05:07", 1));
  const auto ep6(Endpoint::createBt("01:02:03:04:05:07", 1));
  const auto ep7(Endpoint::createIpV4());

  EXPECT_NE(ep0, ep1);
  EXPECT_EQ(ep1, ep2);
  EXPECT_EQ(ep3, ep4);
  EXPECT_EQ(ep5, ep6);
  EXPECT_NE(ep1, ep3);
  EXPECT_NE(ep1, ep5);
  EXPECT_NE(ep3, ep5);
  EXPECT_NE(ep7, ep4);

  ep3 = ep6;
  EXPECT_EQ(ep3, ep6);
  EXPECT_NE(ep3, ep4);

  auto handle1 = ep1.nativeHandle();
  auto handle2 = ep3.nativeHandle();
  std::memcpy(handle1.data(), handle2.data(), handle2.size());
  EXPECT_EQ(ep3, ep1);

  EXPECT_DEATH(Endpoint::createBt(":"), "");
}

TEST(Net, TCPOperationsSome) {
  exec::async_scope scope;
  heph::concurrency::Context context{ {} };
  const auto acceptor = Acceptor::createTcpIpV4(context);

  acceptor.bind(Endpoint::createIpV4());
  acceptor.listen();

  auto endpoint = acceptor.localEndpoint();

  static constexpr std::size_t MSG_SIZE = 4ull * 1024 * 1024;
  std::vector<char> recv_buffer(MSG_SIZE);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
  auto server_sender = [&]() -> exec::task<void> {
    auto client = co_await accept(acceptor);
    auto buffer = std::as_writable_bytes(std::span{ recv_buffer });

    while (!buffer.empty()) {
      auto recvd_buffer = co_await recv(client, buffer);
      EXPECT_LE(buffer.size(), recv_buffer.size());
      buffer = buffer.subspan(recvd_buffer.size());
    }
    context.requestStop();
  };

  scope.spawn(server_sender());

  const auto client{ Socket::createTcpIpV4(context) };
  client.connect(endpoint);

  std::vector<char> send_buffer(MSG_SIZE);
  std::iota(send_buffer.begin(), send_buffer.end(), 0);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
  auto client_sender = [&]() -> exec::task<void> {
    auto buffer = std::as_bytes(std::span{ send_buffer });

    while (!buffer.empty()) {
      auto sent_buffer = co_await send(client, buffer);
      EXPECT_LE(buffer.size(), send_buffer.size());
      buffer = buffer.subspan(sent_buffer.size());
    }
  };
  scope.spawn(client_sender());

  EXPECT_NE(recv_buffer, send_buffer);

  context.run();
  EXPECT_EQ(recv_buffer, send_buffer);
}

TEST(Net, TCPOperationsAll) {
  exec::async_scope scope;
  heph::concurrency::Context context{ {} };
  const auto acceptor = Acceptor::createTcpIpV4(context);

  acceptor.bind(Endpoint::createIpV4());
  acceptor.listen();

  auto endpoint = acceptor.localEndpoint();

  static constexpr std::size_t MSG_SIZE = 4ull * 1024 * 1024;
  std::vector<char> recv_buffer(MSG_SIZE);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
  auto server_sender = [&]() -> exec::task<void> {
    auto client = co_await accept(acceptor);
    auto buffer = std::as_writable_bytes(std::span{ recv_buffer });

    buffer = co_await recvAll(client, buffer);
    EXPECT_EQ(buffer.size(), recv_buffer.size());
    context.requestStop();
  };

  scope.spawn(server_sender());

  const auto client{ Socket::createTcpIpV4(context) };
  client.connect(endpoint);

  std::vector<char> send_buffer(MSG_SIZE);
  std::iota(send_buffer.begin(), send_buffer.end(), 0);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
  auto client_sender = [&]() -> exec::task<void> {
    auto buffer = std::as_bytes(std::span{ send_buffer });

    buffer = co_await sendAll(client, buffer);
    EXPECT_EQ(buffer.size(), send_buffer.size());
  };
  scope.spawn(client_sender());

  EXPECT_NE(recv_buffer, send_buffer);

  context.run();
  EXPECT_EQ(recv_buffer, send_buffer);
}

TEST(Net, UDPOperations) {
  exec::async_scope scope;
  heph::concurrency::Context context{ {} };
  const auto server{ Socket::createUdpIpV4(context) };

  server.bind(Endpoint::createIpV4());

  auto endpoint = server.localEndpoint();

  static constexpr std::size_t MSG_SIZE = 16ull * 1024;
  std::vector<char> recv_buffer(MSG_SIZE);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
  auto server_sender = [&]() -> exec::task<void> {
    auto buffer =
        co_await (recv(server, std::as_writable_bytes(std::span{ recv_buffer })) |
                  stdexec::upon_error([](std::error_code /*ec*/) { return std::span<std::byte>{}; }));
    EXPECT_EQ(buffer.size(), recv_buffer.size());
    context.requestStop();
  };
  scope.spawn(server_sender());

  const auto client{ Socket::createUdpIpV4(context) };
  client.connect(endpoint);

  std::vector<char> send_buffer(MSG_SIZE);
  std::iota(send_buffer.begin(), send_buffer.end(), 0);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
  auto client_sender = [&]() -> exec::task<void> {
    auto buffer = co_await send(client, std::as_bytes(std::span{ send_buffer }));
    EXPECT_EQ(buffer.size(), send_buffer.size());
  };
  scope.spawn(client_sender());

  EXPECT_NE(recv_buffer, send_buffer);

  context.run();
  EXPECT_EQ(recv_buffer, send_buffer);
}
}  // namespace heph::net
