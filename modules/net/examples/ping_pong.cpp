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
auto pong(heph::concurrency::ContextScheduler scheduler, heph::net::Socket socket) -> exec::task<void> {
  std::array<char, PAGE_SIZE> buffer{};
  while (true) {
    auto received = co_await (scheduler.schedule() |
                              heph::net::recv(socket, std::as_writable_bytes(std::span{ buffer })));

    fmt::print("{}", std::string_view(buffer.data(), received.size()));

    co_await (scheduler.schedule() | heph::net::sendAll(socket, received));
  }
}

auto server(heph::concurrency::ContextScheduler scheduler, heph::net::Acceptor acceptor) -> exec::task<void> {
  exec::async_scope scope;

  while (true) {
    auto socket = co_await (scheduler.schedule() | heph::net::accept(acceptor));
    scope.spawn(pong(scheduler, std::move(socket)));
  }
}

auto ping(unsigned i, heph::concurrency::ContextScheduler scheduler, heph::net::Endpoint endpoint)
    -> exec::task<void> {
  const heph::net::Socket socket(heph::net::IpFamily::V4, heph::net::Protocol::TCP);
  socket.connect(endpoint);

  for (unsigned j = 0;; ++j) {
    auto message = fmt::format("Ping {}: {} ", i, j);
    co_await (scheduler.schedule() | heph::net::sendAll(socket, std::as_bytes(std::span{ message })));

    co_await (scheduler.schedule() |
              heph::net::recvAll(socket, std::as_writable_bytes(std::span{ message })));
    fmt::println("Pong {}: {}", i, j);

    static constexpr std::chrono::seconds WAIT_TIME{ 5 };
    co_await (scheduler.scheduleAfter(WAIT_TIME));
  }
}

}  // namespace

auto main(int argc, const char* argv[]) -> int {
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("Ping Pong example");
    desc.defineOption<unsigned>("num_clients", "Number of clients to talk to server concurrently", 1);
    const auto args = std::move(desc).parse(argc, argv);

    const auto num_clients = args.getOption<unsigned>("num_clients");

    heph::concurrency::Context context{ {} };

    heph::net::Acceptor acceptor{ heph::net::IpFamily::V4, heph::net::Protocol::TCP };
    acceptor.bind(heph::net::Endpoint(heph::net::IpFamily::V4, "127.0.0.1"));
    acceptor.listen();
    auto endpoint = acceptor.localEndpoint();
    fmt::println("Server running on {}", endpoint);

    exec::async_scope scope;

    scope.spawn(server(context.scheduler(), std::move(acceptor)));

    for (unsigned i = 0; i != num_clients; ++i) {
      scope.spawn(ping(i, context.scheduler(), endpoint));
    }

    context.run();

    return 0;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return 1;
  }
}
