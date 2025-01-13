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
  fmt::println("test: {}", x);
  EXPECT_NE(formatted.find("test_value"), std::string::npos);
}

}  // namespace heph::format::tests
