//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>

#include <fmt/base.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <hephaestus/utils/concepts.h>
#include <hephaestus/utils/format/format.h>
#include <rfl.hpp>

#include "hephaestus/format/generic_formatter.h"

// NOLINTNEXTLINE(google-build-using-namespace)

using namespace ::testing;

namespace hephaestus::format::tests {

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
    forkify::types::TimestampT b = forkify::types::ClockT::now();
  };
  TestStruct x{};
  std::string formatted = toString(x);
  EXPECT_NE(formatted.find("test_value"), std::string::npos);
}

TEST(GenericFormatterTests, TestFormatEmptyPoint) {
  forkify::types::sick_lidar::LidarPoint2 empty_scan;
  std::string formatted = toString(empty_scan);
  EXPECT_FALSE(formatted.empty());
}

TEST(GenericFormatterTests, TestFormatEmptyPointProperties) {
  forkify::types::sick_lidar::LidarPoint2Properties empty_scan;
  std::string formatted = toString(empty_scan);
  EXPECT_FALSE(formatted.empty());
}

TEST(GenericFormatterTests, TestFormatEmptyObject) {
  forkify::types::sick_lidar::DualLidarScan2Merged empty_scan;
  std::string formatted = toString(empty_scan);
  EXPECT_FALSE(formatted.empty());
}

TEST(GenericFormatterTests, TestFormatMultipleObjects) {
  std::mt19937_64 mt{ std::random_device{}() };
  auto random_scan1 = generateRandom<types::sick_lidar::DualLidarScan2Merged>(mt);
  auto random_scan2 = generateRandom<types::sick_lidar::DualLidarScan2Merged>(mt);

  std::string formatted1 = toString(random_scan1);
  std::string formatted2 = toString(random_scan2);

  EXPECT_NE(formatted1, formatted2);
}
TEST(GenericFormatterTests, TestFormatConsistency) {
  types::sick_lidar::DualLidarScan2Merged scan1;
  types::sick_lidar::DualLidarScan2Merged scan2;
  {
    std::mt19937_64 mt{ 42 };  // Fixed seed for consistency
    scan1 = generateRandom<types::sick_lidar::DualLidarScan2Merged>(mt);
  }
  {
    std::mt19937_64 mt{ 42 };  // Fixed seed for consistency
    scan2 = generateRandom<types::sick_lidar::DualLidarScan2Merged>(mt);
  }

  std::string formatted1 = toString(scan1);
  std::string formatted2 = toString(scan2);

  std::println("test: general fmt formatter:\n {}", scan1);

  EXPECT_EQ(formatted1, formatted2);
  EXPECT_TRUE(false);
}

}  // namespace hephaestus::format::tests
