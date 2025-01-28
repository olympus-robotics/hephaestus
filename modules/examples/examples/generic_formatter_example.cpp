//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <string>

#include <fmt/base.h>

#include "hephaestus/format/generic_formatter.h"  // NOLINT(misc-include-cleaner)

struct MyStruct {
  int a = 42;  // NOLINT(readability-magic-numbers)
  std::string b = "The answer to life, the universe, and everything";
  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> d =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
};
auto main() -> int {
  MyStruct x{};
  fmt::println("My struct is:\n{}", x);

  return EXIT_SUCCESS;
}
