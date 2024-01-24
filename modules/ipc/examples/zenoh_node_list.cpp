//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <print>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/ipc/zenoh/scout.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;

  try {
    std::println("Scouting..");

    auto nodes_info = eolo::ipc::zenoh::getListOfNodes();
    std::ranges::for_each(nodes_info,
                          [](const auto& info) { std::println("{}", eolo::ipc::zenoh::toString(info)); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
  }
}
