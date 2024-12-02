//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <tuple>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/cli/program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    // describe the program and all it's command line options
    auto desc = heph::cli::ProgramDescription("A dummy service that does nothing");
    desc.defineOption<int>("port", 'p', "The port this service is available on")
        .defineOption<std::string>("address", "The IP address of this service", "[::]")
        .defineFlag("broadcast", 'b', "enable broadcast");

    // parse the command line arguments
    const auto args = std::move(desc).parse(argc, argv);
    const auto port = args.getOption<int>("port");
    const auto address = args.getOption<std::string>("address");
    const auto broadcast = args.getOption<bool>("broadcast");

    // help is always available. Specify '--help' on command line or get it directly as here.
    fmt::println("Help text:\n{}\n", args.getOption<std::string>("help"));

    // print the arguments passed
    fmt::println("You specified port = {}", port);
    fmt::println("The IP address in use is {}", address);
    fmt::println("Broadcasting is enabled {}", broadcast);

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
