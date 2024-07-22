//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <format>
#include <iomanip>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "hephaestus/telemetry/struclog.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace hephtelemetry::tests {

namespace ht = heph::telemetry;
// NOLINTBEGIN google-build-using-namespace
using namespace heph::telemetry::literals;
// NOLINTEND google-build-using-namespace

auto printout(const ht::Log& s) -> std::string {
  return s.format();
}

TEST(struclog, Log) {
  std::string a = "test a great message";
  std::string b = "test \"great\" name";
  // clang-format off
  int current_line = __LINE__; auto s = printout(ht::Log{std::string(a)} | "b"_f("test \"great\" name"));
  // Old format
  // int current_line = __LINE__; auto s = printout(ht::Log{std::string(a)}("b", std::string(b)));
  // clang-format on

  std::stringstream ss;
  ss << "message=" << std::quoted(a);

  std::stringstream ss2;
  ss2 << " b=" << std::quoted(b);

  EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  EXPECT_TRUE(s.find(std::format("location=\"tests.cpp:{}\"", current_line)) != std::string::npos);
  EXPECT_TRUE(s.find(ss2.str()) != std::string::npos);
}

TEST(struclog, Escapes) {
  const std::string a = "test a great message";
  const std::string b = "test \"great\" name";
  const std::string c = "test 'great' name";
  const int num = 123;
  // clang-format off
  int current_line = __LINE__; auto s = printout(ht::Log{a} | "b"_f(b) | ht::Field{"c", c} | "num"_f(num));
  // Old format:
  // int current_line = __LINE__; auto s = printout(ht::Log{ std::string(a) }("b", std::string(b))( "c", std::string(c) )( "num", num));
  // clang-format on

  std::stringstream ss;
  ss << "message=" << std::quoted(a) << " ";
  ss << std::format("location=\"tests.cpp:{}\"", current_line) << " ";
  ss << "b=" << std::quoted(b) << " ";
  ss << "c=" << std::quoted(c) << " ";
  ss << "num=" << num;

  EXPECT_EQ(s, ss.str());
}

TEST(Example, Example) {
  EXPECT_TRUE(true);
}
}  // namespace hephtelemetry::tests
