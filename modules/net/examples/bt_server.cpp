//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <fmt/base.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

namespace {
inline constexpr std::size_t PACKET_SIZE = 65535;
inline constexpr double KB = 1024.;
// NOLINTNEXTLINE (readability-static-accessed-through-instance)
auto pong(heph::concurrency::ContextScheduler scheduler,
          heph::net::Socket socket) -> exec::task<void> {
  std::array<char, PACKET_SIZE> buffer{};

  while (true) {
    std::vector<char> message;
    message.reserve(PACKET_SIZE * 2);

    {
      auto begin = std::chrono::high_resolution_clock::now();
      while (true) {
        auto received =
            co_await (scheduler.schedule() |
                      heph::net::recvAll(
                          socket, std::as_writable_bytes(std::span{buffer})));
        message.insert(message.end(), buffer.begin(),
                       buffer.begin() + received.size());
        if (message.back() == 'e') {
          break;
        }
      }
      auto end = std::chrono::high_resolution_clock::now();
      const std::chrono::duration<double> duration = end - begin;

      fmt::println(stderr, "Receive, {:.2f}s, {:.2f} KB/s", duration.count(),
                   (static_cast<double>(message.size()) / KB) /
                       duration.count());
    }

    {
      auto send_buffer = std::as_bytes(std::span{message}).subspan(0, 1);
      co_await (scheduler.schedule() | heph::net::sendAll(socket, send_buffer));
    }
  }
}

// NOLINTNEXTLINE (readability-static-accessed-through-instance)
auto server(heph::concurrency::ContextScheduler scheduler,
            heph::net::Acceptor acceptor) -> exec::task<void> {
  exec::async_scope scope;

  // NOLINTNEXTLINE
  while (true) {
    auto socket = co_await (scheduler.schedule() | heph::net::accept(acceptor));
    scope.spawn(pong(scheduler, std::move(socket)));
  }
}

} // namespace

auto main(int argc, const char *argv[]) -> int {
  heph::telemetry::registerLogSink(
      std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("BT server");
    desc.defineOption<std::string>("address", "Bluetooth adapter to use");
    const auto args = std::move(desc).parse(argc, argv);

    const auto address = args.getOption<std::string>("address");

    heph::concurrency::Context context{{}};

    heph::net::Acceptor acceptor{heph::net::IpFamily::BT,
                                 heph::net::Protocol::BT};
    acceptor.bind(heph::net::Endpoint(heph::net::IpFamily::BT, address));
    acceptor.listen();
    auto endpoint = acceptor.localEndpoint();
    fmt::println("Server running on {}", endpoint);

    exec::async_scope scope;

    scope.spawn(server(context.scheduler(), std::move(acceptor)));

    context.run();

    return 0;
  } catch (const std::exception &ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return 1;
  }
}
