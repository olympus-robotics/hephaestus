//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/zenoh/scout.h"
#include "hephaestus/utils/stack_trace.h"

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;

  heph::utils::StackTrace stack_trace;

  try {
    fmt::println("Scouting..");

    auto nodes_info = heph::ipc::zenoh::getListOfNodes();
    std::for_each(nodes_info.begin(), nodes_info.end(),
                  [](const auto& info) { fmt::println("{}", heph::ipc::zenoh::toString(info)); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
  }
}
