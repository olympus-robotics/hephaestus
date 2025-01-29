//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <exception>
#include <string>

#include <fmt/base.h>

#include "hephaestus/format/generic_formatter.h"  // NOLINT(misc-include-cleaner)

struct MyStruct {
  int a = 42;  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  std::string b = "The answer to life, the universe, and everything";
  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> d =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
};
auto main() -> int {
  try {
    MyStruct x{};
    fmt::println("My struct is:\n{}", x);
  } catch (const std::exception& e) {
    fmt::println("Exception: {}\n", e.what());
  }

  return EXIT_SUCCESS;
}
