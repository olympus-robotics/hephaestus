//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/program_options.h"

#include "hephaestus/cli/program_options.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc {

void appendIPCProgramOption(cli::ProgramDescription& program_description) {
  static constexpr auto DEFAULT_KEY = "**";

  program_description.defineOption<std::string>("topic", 't', "Key expression", DEFAULT_KEY)
      .defineOption<std::size_t>("cache", 'c', "Cache size", 0)
      .defineOption<std::string>("mode", 'm', "Running mode: options: peer, client", "peer")
      .defineOption<std::string>("router", 'r', "Router endpoint", "")
      .defineOption<std::string>("protocol", 'p', "Protocol to use, options: udp, tcp, any", "any")
      .defineFlag("shared_memory", 's', "Enable shared memory")
      .defineFlag("qos", 'q', "Enable QoS")
      .defineFlag("realtime", "Enable real-time communication");
}

auto parseIPCProgramOptions(const heph::cli::ProgramOptions& args) -> std::pair<Config, TopicConfig> {
  TopicConfig topic_config{ .name = args.getOption<std::string>("topic") };

  Config config;
  config.cache_size = args.getOption<std::size_t>("cache");

  auto mode = args.getOption<std::string>("mode");
  if (mode == "peer") {
    config.mode = Mode::PEER;
  } else if (mode == "client") {
    config.mode = Mode::CLIENT;
  } else {
    heph::throwException<heph::InvalidParameterException>(fmt::format("invalid mode value: {}", mode));
  }

  auto protocol = args.getOption<std::string>("protocol");
  if (protocol == "any") {
    config.protocol = Protocol::ANY;
  } else if (protocol == "udp") {
    config.protocol = Protocol::UDP;
  } else if (protocol == "tcp") {
    config.protocol = Protocol::TCP;
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

}  // namespace heph::ipc
