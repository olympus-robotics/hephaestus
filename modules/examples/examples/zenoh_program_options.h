//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/exception.h"

enum class ExampleType : uint8_t { PUBSUB, SERVICE, ACTION_SERVER };

[[nodiscard]] inline auto getProgramDescription(const std::string& description,
                                                const ExampleType type) -> heph::cli::ProgramDescription {
  static constexpr auto DEFAULT_PUBSUB_KEY = "hephaestus/ipc/example/zenoh/put";
  static constexpr auto DEFAULT_SERVICE_KEY = "hephaestus/ipc/example/zenoh/service";
  static constexpr auto DEFAULT_ACTION_SERVER_KEY = "hephaestus/ipc/example/zenoh/action_server";

  const auto* const default_topic = [&type] {
    switch (type) {
      case ExampleType::PUBSUB:
        return DEFAULT_PUBSUB_KEY;
      case ExampleType::SERVICE:
        return DEFAULT_SERVICE_KEY;
      case ExampleType::ACTION_SERVER:
        return DEFAULT_ACTION_SERVER_KEY;
    }
  }();

  auto desc = heph::cli::ProgramDescription(description);
  desc.defineOption<std::string>("topic", 't', "Key expression", default_topic)
      .defineOption<std::size_t>("cache", 'c', "Cache size", 0)
      .defineOption<std::string>("mode", 'm', "Running mode: options: peer, client", "peer")
      .defineOption<std::string>("router", 'r', "Router endpoint", "")
      .defineOption<std::string>("protocol", 'p', "Protocol to use, options: udp, tcp, any", "any")
      .defineFlag("shared_memory", 's', "Enable shared memory")
      .defineFlag("qos", 'q', "Enable QoS")
      .defineFlag("realtime", "Enable real-time communication");

  return desc;
}

[[nodiscard]] inline auto parseArgs(const heph::cli::ProgramOptions& args)
    -> std::pair<heph::ipc::zenoh::Config, heph::ipc::TopicConfig> {
  heph::ipc::TopicConfig topic_config{ .name = args.getOption<std::string>("topic") };

  heph::ipc::zenoh::Config config;
  config.cache_size = args.getOption<std::size_t>("cache");

  auto mode = args.getOption<std::string>("mode");
  if (mode == "peer") {
    config.mode = heph::ipc::zenoh::Mode::PEER;
  } else if (mode == "client") {
    config.mode = heph::ipc::zenoh::Mode::CLIENT;
  } else {
    heph::throwException<heph::InvalidParameterException>(fmt::format("invalid mode value: {}", mode));
  }

  auto protocol = args.getOption<std::string>("protocol");
  if (protocol == "any") {
    config.protocol = heph::ipc::zenoh::Protocol::ANY;
  } else if (protocol == "udp") {
    config.protocol = heph::ipc::zenoh::Protocol::UDP;
  } else if (protocol == "tcp") {
    config.protocol = heph::ipc::zenoh::Protocol::TCP;
  } else {
    heph::throwException<heph::InvalidParameterException>(
        fmt::format("invalid value {} for option 'protocol'", protocol));
  }

  config.router = args.getOption<std::string>("router");
  config.enable_shared_memory = args.getOption<bool>("shared_memory");
  config.qos = args.getOption<bool>("qos");
  config.real_time = args.getOption<bool>("realtime");

  return { std::move(config), std::move(topic_config) };
}
