//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================
#include <string>
#include <thread>

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <gtest/gtest.h>
// THIS DOES NOT PASS
// clang-format off
// #include "hephaestus/format/generic_formatter.h"  // NOLINT(misc-include-cleaner)
// clang-format on

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

struct MyTest {
  // std::string a = "test";
  std::thread::id id = std::this_thread::get_id();
};
template <>
struct fmt::formatter<MyTest> : fmt::formatter<std::string_view> {
  static auto format(const MyTest& data, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "id={}", data.id);
  }
};

// THIS PASSES
// clang-format off
#include "hephaestus/format/generic_formatter.h"  // NOLINT(misc-include-cleaner)
// clang-format on

namespace heph::format::tests {
TEST(FormatterCollisions, TestFormatKnownObject) {
  const MyTest x{};
  std::string formatted = fmt::format("{}", x);
  EXPECT_NE(formatted.find("id"), std::string::npos);
}
}  // namespace heph::format::tests
