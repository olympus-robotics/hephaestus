//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/base/exception.h"
#include "eolo/cli/program_options.h"
#include "eolo/ipc/zenoh/config.h"

[[nodiscard]] inline auto
getProgramDescription(const std::string& description) -> eolo::cli::ProgramDescription {
  static constexpr auto DEFAULT_KEY = "eolo/ipc/example/zenoh/put";

  auto desc = eolo::cli::ProgramDescription(description);
  desc.defineOption<std::string>("topic", 't', "Key expression", DEFAULT_KEY)
      .defineOption<std::size_t>("cache", 'c', "Cache size", 0)
      .defineOption<std::string>("mode", 'm', "Running mode: options: peer, client", "peer")
      .defineOption<std::string>("router", 'r', "Router endpoint", "")
      .defineFlag("shared_memory", 's', "Enable shared memory");

  return desc;
}

[[nodiscard]] inline auto parseArgs(const eolo::cli::ProgramOptions& args) -> eolo::ipc::zenoh::Config {
  eolo::ipc::zenoh::Config config;
  config.topic = args.getOption<std::string>("topic");
  config.cache_size = args.getOption<std::size_t>("cache");
  auto mode = args.getOption<std::string>("mode");
  if (mode == "peer") {
    config.mode = eolo::ipc::zenoh::Config::PEER;
  } else if (mode == "client") {
    config.mode = eolo::ipc::zenoh::Config::CLIENT;
  } else {
    eolo::throwException<eolo::InvalidParameterException>(std::format("invalid mode value: {}", mode));
  }

  config.enable_shared_memory = args.getOption<bool>("shared_memory");
  config.router = args.getOption<std::string>("router");

  return config;
}
