//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/scout.h"

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;

  try {
    fmt::println("Scouting..");

    auto nodes_info = heph::ipc::zenoh::getListOfNodes();
    std::ranges::for_each(nodes_info,
                          [](const auto& info) { fmt::println("{}", heph::ipc::zenoh::toString(info)); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
  }
}
