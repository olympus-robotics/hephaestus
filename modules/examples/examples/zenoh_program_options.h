//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/base/exception.h"
#include "eolo/cli/program_options.h"
#include "eolo/ipc/common.h"

[[nodiscard]] inline auto
getProgramDescription(const std::string& description) -> eolo::cli::ProgramDescription {
  static constexpr auto DEFAULT_KEY = "eolo/ipc/example/zenoh/put";

  auto desc = eolo::cli::ProgramDescription(description);
  desc.defineOption<std::string>("topic", 't', "Key expression", DEFAULT_KEY)
      .defineOption<std::size_t>("cache", 'c', "Cache size", 0)
      .defineOption<std::string>("mode", 'm', "Running mode: options: peer, client", "peer")
      .defineOption<std::string>("router", 'r', "Router endpoint", "")
      .defineOption<std::string>("protocol", 'p', "Protocol to use, options: udp, tcp, any", "any")
      .defineFlag("shared_memory", 's', "Enable shared memory")
      .defineFlag("qos", 'q', "Enable QoS")
      .defineFlag("realtime", "Enable real-time communication");

  return desc;
}

[[nodiscard]] inline auto
parseArgs(const eolo::cli::ProgramOptions& args) -> std::pair<eolo::ipc::Config, eolo::ipc::TopicConfig> {
  eolo::ipc::TopicConfig topic_config{ .name = args.getOption<std::string>("topic") };

  eolo::ipc::Config config;
  config.cache_size = args.getOption<std::size_t>("cache");

  auto mode = args.getOption<std::string>("mode");
  if (mode == "peer") {
    config.mode = eolo::ipc::Mode::PEER;
  } else if (mode == "client") {
    config.mode = eolo::ipc::Mode::CLIENT;
  } else {
    eolo::throwException<eolo::InvalidParameterException>(std::format("invalid mode value: {}", mode));
  }

  auto protocol = args.getOption<std::string>("protocol");
  if (protocol == "any") {
    config.protocol = eolo::ipc::Protocol::ANY;
  } else if (protocol == "udp") {
    config.protocol = eolo::ipc::Protocol::UDP;
  } else if (protocol == "tcp") {
    config.protocol = eolo::ipc::Protocol::TCP;
  } else {
    eolo::throwException<eolo::InvalidParameterException>(
        std::format("invalid value {} for option 'protocol'", protocol));
  }

  config.router = args.getOption<std::string>("router");
  config.enable_shared_memory = args.getOption<bool>("shared_memory");
  config.qos = args.getOption<bool>("qos");
  config.real_time = args.getOption<bool>("realtime");

  return { std::move(config), std::move(topic_config) };
}
