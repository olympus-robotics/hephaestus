//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <chrono>
#include <exception>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <hephaestus/concurrency/context_scheduler.h>
#include <hephaestus/net/endpoint.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

namespace {
inline constexpr unsigned PAGE_SIZE = 4096;
auto pong(heph::net::Socket socket) -> exec::task<void> {
  std::array<char, PAGE_SIZE> buffer{};
  while (true) {
    auto received = co_await heph::net::recv(socket, std::as_writable_bytes(std::span{ buffer }));

    fmt::print("{}", std::string_view(buffer.data(), received.size()));

    // co_await heph::net::sendAll(socket, received);
  }
}

[[maybe_unused]] auto server(heph::net::Acceptor acceptor) -> exec::task<void> {
  exec::async_scope scope;
  fmt::println(stderr, "server");
  while (true) {
    auto socket = co_await heph::net::accept(acceptor);
    fmt::println(stderr, "socket: {}", socket.localEndpoint());
    scope.spawn(pong(std::move(socket)));
  }
}

[[maybe_unused]] auto ping(heph::concurrency::ContextScheduler scheduler, heph::net::Endpoint endpoint)
    -> exec::task<void> {
  fmt::println(stderr, "ping endpoint: {}", endpoint);
  const auto socket = heph::net::Socket::createSocketcan(scheduler.context());
  fmt::println(stderr, "ping socket");
  socket.connect(endpoint);
  fmt::println(stderr, "ping connect");

  for (unsigned j = 0;; ++j) {
    // auto message = fmt::format("Ping: {} ", j);
    canfd_frame frame{};
    frame.can_id = 0;
    frame.len = CANFD_MAX_DLEN;
    std::memset(&frame.data[0], 0x50, CANFD_MAX_DLEN);
    frame.data[0] = 42;

    fmt::println(stderr, "ping write");
    try {
      co_await heph::net::sendAll(socket, std::as_bytes(std::span{ &frame, sizeof(frame) }));
    } catch (std::exception& e) {
      fmt::println(stderr, "exception: {}", e.what());
    }
    fmt::println(stderr, "ping write DONE");

    // co_await heph::net::recvAll(socket, std::as_writable_bytes(std::span{ &frame, sizeof(frame) }));
    fmt::println("Pong: {}", j);

    static constexpr std::chrono::seconds WAIT_TIME{ 5 };
    co_await (scheduler.scheduleAfter(WAIT_TIME));
  }
}
}  // namespace

auto main(int, const char*[]) -> int {
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("SocketCAN Ping Pong example");

    heph::concurrency::Context context{ {} };

    auto acceptor = heph::net::Acceptor::createSocketcan(context);
    auto endpoint = heph::net::Endpoint::createSocketcan("vcan0");
    acceptor.bind(endpoint);
    // acceptor.listen();
    // auto endpoint = acceptor.localEndpoint();  // TODO: fix this
    fmt::println("Server running on {}", endpoint);

    exec::async_scope scope;
    // fmt::println("Spawn server");
    // scope.spawn(server(std::move(acceptor)));

    fmt::println("Spawn ping");
    scope.spawn(ping(context.scheduler(), endpoint));

    context.run();

    return 0;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return 1;
  }
}
