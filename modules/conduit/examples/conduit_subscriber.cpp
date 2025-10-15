
//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstdint>
#include <exception>
#include <memory>
#include <string_view>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/conduit/remote_output_subscriber.h"
#include "hephaestus/format/generic_formatter.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/net/endpoint.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/utils/signal_handler.h"

struct Sink : heph::conduit::Node<Sink> {
  static constexpr std::string_view NAME = "sink";
  heph::conduit::QueuedInput<heph::types::DummyType> input{ this, "input" };

  static auto trigger(Sink& self) {
    return self.input.get();
  }

  static auto execute(heph::types::DummyType dummy) {
    fmt::println("Received {}", dummy);
  }
};

auto main(int argc, const char* argv[]) -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc = heph::cli::ProgramDescription("Conduit Publisher");
    desc.defineOption<std::string>("address", "Address to connect to", "127.0.0.1");
    desc.defineOption<std::uint16_t>("port", "Port to connect to");

    const auto args = std::move(desc).parse(argc, argv);

    const auto address = args.getOption<std::string>("address");
    const auto port = args.getOption<std::uint16_t>("port");

    auto endpoint = heph::net::Endpoint::createIpV4(address, port);

    heph::conduit::NodeEngine engine{ {} };

    fmt::println("Subscribing to {}", endpoint);

    auto subscriber = engine.createNode<heph::conduit::RemoteOutputSubscriber<heph::types::DummyType>>(
        endpoint, std::string{ "generator" });
    auto node = engine.createNode<Sink>();

    node->input.connectTo(subscriber);

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine]() { engine.requestStop(); });

    engine.run();

    return 0;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return 1;
  }
}
