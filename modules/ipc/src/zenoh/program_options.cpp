//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/program_options.h"

#include <cstddef>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

void appendProgramOption(cli::ProgramDescription& program_description,
                         const std::string& default_topic /*= DEFAULT_TOPIC*/) {
  program_description.defineOption<std::string>("topic", "Key expression", default_topic)
      .defineOption<std::string>("session_id", "the session id", "")
      .defineOption<std::size_t>("cache", "Cache size", 0)
      .defineOption<std::string>("mode", "Running mode: options: peer, client", "peer")
      .defineOption<std::string>("router", "Router endpoint", "")
      .defineOption<std::string>("protocol", "Protocol to use, options: udp, tcp, any", "any")
      .defineOption<std::string>("multicast_scouting_interface",
                                 "Interface to use for multicast, options: auto, <INTERFACE_NAME>", "auto")
      .defineFlag("shared_memory", "Enable shared memory")
      .defineFlag("multicast_scouting_enabled", "Enable multicast scouting")
      .defineFlag("qos", "Enable QoS")
      .defineFlag("realtime", "Enable real-time communication");
}

auto parseProgramOptions(const heph::cli::ProgramOptions& args) -> std::pair<Config, TopicConfig> {
  TopicConfig topic_config{ args.getOption<std::string>("topic") };

  Config config;
  if (auto session_id = args.getOption<std::string>("session_id"); !session_id.empty()) {
    config.id = std::move(session_id);
  }

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
  config.multicast_scouting_enabled = args.getOption<bool>("multicast_scouting_enabled");
  config.multicast_scouting_interface = args.getOption<std::string>("multicast_scouting_interface");
  config.qos = args.getOption<bool>("qos");
  config.real_time = args.getOption<bool>("realtime");

  return { std::move(config), std::move(topic_config) };
}

}  // namespace heph::ipc::zenoh
