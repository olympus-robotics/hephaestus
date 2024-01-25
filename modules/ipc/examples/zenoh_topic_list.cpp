//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <algorithm>
#include <print>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/ipc/zenoh/liveliness.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Periodic publisher example");
    const auto args = std::move(desc).parse(argc, argv);

    auto config = parseArgs(args);

    std::println("Opening session...");
    const auto publishers_info = eolo::ipc::zenoh::getListOfPublishers(std::move(config));
    std::ranges::for_each(publishers_info,
                          [](const auto& info) { eolo::ipc::zenoh::printPublisherInfo(info); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
