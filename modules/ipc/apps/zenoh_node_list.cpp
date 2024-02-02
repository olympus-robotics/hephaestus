//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/ipc/zenoh/scout.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;

  try {
    fmt::println("Scouting..");

    auto nodes_info = eolo::ipc::zenoh::getListOfNodes();
    std::ranges::for_each(nodes_info,
                          [](const auto& info) { fmt::println("{}", eolo::ipc::zenoh::toString(info)); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
  }
}
