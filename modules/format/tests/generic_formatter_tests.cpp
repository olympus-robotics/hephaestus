//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>

#include <fmt/base.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <rfl.hpp>

#include "hephaestus/format/generic_formatter.h"
#include "hephaestus/types/bounds.h"

// NOLINTNEXTLINE(google-build-using-namespace)

using namespace ::testing;

namespace heph::format::tests {

TEST(GenericFormatterTests, TestFormatInt) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  int x = 42;
  std::string formatted = toString(x);
  EXPECT_NE(formatted.find("42"), std::string::npos);
}

TEST(GenericFormatterTests, TestFormatKnownObject) {
  struct TestStruct {
    std::string a;
  };
  TestStruct x{ .a = "test_value" };
  std::string formatted = toString(x);
  EXPECT_NE(formatted.find("test_value"), std::string::npos);
}

TEST(GenericFormatterTests, TestFormatKnownObjectWithRflTimestamp) {
  static constexpr std::array<char, 18> timestamp_format{ "%Y-%m-%d %H:%M:%S" };
  struct TestStruct {
    std::string a = "test_value";
    rfl::Timestamp<timestamp_format> b;
  };
  TestStruct x{};
  auto timestamp = std::chrono::system_clock::now();
  x.b = fmt::format(
      "{:%Y-%m-%d %H:%M:%S}",
      std::chrono::time_point_cast<std::chrono::microseconds, std::chrono::system_clock>(timestamp));
  std::string formatted = toString(x);
  EXPECT_NE(formatted.find("test_value"), std::string::npos);
}

TEST(GenericFormatterTests, TestFormatKnownObjectWithChrono) {
  struct TestStruct {
    std::string a = "test_value";
    std::chrono::time_point<std::chrono::system_clock> b = std::chrono::system_clock::now();
  };
  TestStruct x{};
  std::string formatted = toString(x);
  EXPECT_NE(formatted.find("test_value"), std::string::npos);

  // Dummy test if general printers can be compiled
  std::cout << "cout: " << x << "\n" << std::flush;
  fmt::println("fmt: {}", x);
}

TEST(GenericFormatterTests, TestFormatBounds) {
  types::Bounds<int> bounds{ .lower = 1, .upper = 2, .type = types::BoundsType::INCLUSIVE };
  types::Bounds<int> bounds2{ .lower = 3, .upper = 4, .type = types::BoundsType::LEFT_OPEN };
  std::string formatted = toString(bounds);
  fmt::println("bounds inclusive: {} vs\n {}", bounds, formatted);
  fmt::println("bounds left open: {} vs\n {}", bounds2, toString(bounds2));

  EXPECT_NE(formatted.find("1"), std::string::npos);
  EXPECT_NE(formatted.find("2"), std::string::npos);
}

TEST(GenericFormatterTests, TestFormatStructWithBounds) {
  struct TestStruct {
    types::Bounds<int> bounds{ .lower = 1, .upper = 2, .type = types::BoundsType::INCLUSIVE };
    types::Bounds<int> bounds2{ .lower = 3, .upper = 4, .type = types::BoundsType::LEFT_OPEN };
  };
  TestStruct x{};
  std::string formatted = toString(x);

  EXPECT_NE(formatted.find("1"), std::string::npos);
  EXPECT_NE(formatted.find("2"), std::string::npos);
  EXPECT_NE(formatted.find("3"), std::string::npos);
  EXPECT_NE(formatted.find("4"), std::string::npos);

  // Dummy test if the custom formatters can be compiled
  fmt::println("test: {}", x);
  std::cout << "cout: " << x << "\n" << std::flush;
}

}  // namespace heph::format::tests
