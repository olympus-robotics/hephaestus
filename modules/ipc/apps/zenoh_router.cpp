//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <atomic>
#include <csignal>
#include <print>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/cli/program_options.h"
#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/utils.h"

namespace {

std::atomic_flag signal_exit = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void onSignal(int /*signum*/) {
  signal_exit.test_and_set();
  signal_exit.notify_one();
}

// NOLINTNEXTLINE§(cppcoreguidelines-avoid-c-arrays)
[[nodiscard]] auto parseArgs(int argc, const char* argv[]) -> std::pair<int, std::string> {
  static constexpr auto DEFAULT_PORT = 7447;
  static constexpr auto DEFAULT_ADDRESS = "[::]";  //!< all available interfaces

  auto desc = eolo::cli::ProgramDescription("Zenoh Router");
  desc.defineOption<int>("port", 'p', "Port on which the service is available", DEFAULT_PORT)
      .defineOption<std::string>("address", 'a', "IP address of the service", DEFAULT_ADDRESS);
  const auto args = std::move(desc).parse(argc, argv);

  auto port = args.getOption<int>("port");
  auto address = args.getOption<std::string>("address");

  return { port, std::move(address) };
}
}  // namespace

auto main(int argc, const char* argv[]) -> int {
  try {
    std::ignore = signal(SIGINT, onSignal);
    std::ignore = signal(SIGTERM, onSignal);

    const auto [port, address] = parseArgs(argc, argv);

    zenohc::Config config;
    if (not config.insert_json(Z_CONFIG_MODE_KEY, R"("router")")) {
      std::println("Setting mode failed");
      return EXIT_FAILURE;
    }

    const auto listener_endpoint = std::format(R"(["tcp/{}:{}"])", address, port);
    if (not config.insert_json(Z_CONFIG_LISTEN_KEY, listener_endpoint.c_str())) {
      std::println("Setting listening to {} failed", listener_endpoint);
      return EXIT_FAILURE;
    }

    auto session = eolo::ipc::zenoh::expect(open(std::move(config)));
    std::println("Router {} listening on {}", eolo::ipc::zenoh::toString(session.info_zid()),
                 listener_endpoint);

    session.info_routers_zid(
        [](const zenohc::Id& id) { std::println("\t[Router]: {}", eolo::ipc::zenoh::toString(id)); });

    session.info_peers_zid(
        [](const zenohc::Id& id) { std::println("\t[Peer]: {}", eolo::ipc::zenoh::toString(id)); });

    signal_exit.wait(false);

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(std::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
