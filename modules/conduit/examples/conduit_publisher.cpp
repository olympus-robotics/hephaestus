//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <random>
#include <string_view>
#include <utility>

#include <fmt/base.h>
#include <fmt/ranges.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/utils/signal_handler.h"

struct Generator : heph::conduit::Node<Generator> {
  static constexpr std::string_view NAME = "generator";

  static constexpr std::chrono::seconds PERIOD{ 1 };
  std::random_device rd;
  std::mt19937_64 mt{ rd() };

  static auto execute(Generator& self) {
    return heph::types::DummyType::random(self.mt);
  }
};

auto main(int argc, const char* argv[]) -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc = heph::cli::ProgramDescription("Conduit Publisher");
    desc.defineOption<std::string>("address", "Address to bind to", "127.0.0.1");
    desc.defineOption<std::uint16_t>("port", "Port to bind to", 0);

    const auto args = std::move(desc).parse(argc, argv);

    const auto address = args.getOption<std::string>("address");
    const auto port = args.getOption<std::uint16_t>("port");

    heph::conduit::NodeEngineConfig config;
    config.endpoints = { heph::net::Endpoint::createIpV4(address, port) };

    heph::conduit::NodeEngine engine{ config };

    fmt::println("Publisher listening on {}", fmt::join(engine.endpoints(), ","));

    engine.createNode<Generator>();

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine]() { engine.requestStop(); });

    engine.run();

    return 0;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return 1;
  }
}
