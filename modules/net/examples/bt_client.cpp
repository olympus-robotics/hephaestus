//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
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
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

namespace {
// NOLINTNEXTLINE
std::atomic<std::size_t> clients_left{ 0 };

auto ping(heph::concurrency::ContextScheduler scheduler, heph::net::Endpoint endpoint) -> exec::task<void> {
  const auto socket = heph::net::Socket::createL2cap(scheduler.context());

  socket.connect(endpoint);

  for (const std::size_t i :
       { 1ull, 2ull, 4ull, 8ull, 16ull, 32ull, 64ull, 128ull, 256ull, 512ull, 672ull, 1024ull, 4048ull,
         8192ull, 16384ull, 32768ull, 65536ull, 131072ull, 1'048'576ull }) {
    std::vector<char> message(i);
    message.back() = 'e';

    auto begin = std::chrono::high_resolution_clock::now();
    static constexpr std::size_t NUM_ITERATIONS = 1;
    for (std::size_t j = 0; j != NUM_ITERATIONS; ++j) {
      auto send_buffer = std::as_bytes(std::span{ message });
      auto recv_buffer = std::as_writable_bytes(std::span{ message }).subspan(0, 1);

      co_await heph::net::sendAll(socket, send_buffer);
      co_await heph::net::recvAll(socket, recv_buffer);
    }
    auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - begin;
    static constexpr double KB = 1024.;
    const auto throughput = (static_cast<double>(message.size() * NUM_ITERATIONS) / duration.count()) / KB;
    fmt::println(stderr, "Bytes: {}, Duration: {:.2f}s, {:.2f}KB/s", message.size(),
                 duration.count() / NUM_ITERATIONS, throughput);
  }
  auto prev = clients_left.fetch_sub(1);
  if (prev == 1) {
    scheduler.context().requestStop();
  }
}
}  // namespace

auto main(int argc, const char* argv[]) -> int {
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("BT client");
    desc.defineOption<std::string>("address", "Bluetooth adapter to connect to");
    desc.defineOption<std::uint16_t>("port", "Bluetooth port");
    desc.defineOption<std::size_t>("num_clients", "Number of concurrent clients", 1);
    const auto args = std::move(desc).parse(argc, argv);

    const auto address = args.getOption<std::string>("address");
    const auto port = args.getOption<std::uint16_t>("port");
    const auto num_clients = args.getOption<std::size_t>("num_clients");
    clients_left = num_clients;

    heph::concurrency::Context context{ {} };

    auto endpoint = heph::net::Endpoint::createBt(address, port);

    exec::async_scope scope;

    for (std::size_t i = 0; i != num_clients; ++i) {
      scope.spawn(ping(context.scheduler(), endpoint));
    }

    context.run();

    return 0;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return 1;
  }
}
