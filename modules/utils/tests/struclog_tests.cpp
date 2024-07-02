//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <random>

#include <gtest/gtest.h>

#include "hephaestus/utils/struclog.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace sl = heph::utils::struclog;

auto printout(const sl::Log& s) -> std::string {
  return s.format();
}

TEST(struclog, Log) {
  std::string a = "test a great message";
  std::string b = "test \"great\" name";
  // clang-format off
  int current_line = __LINE__; auto s = printout(sl::Log(std::string(a))("b", std::string(b)));
  // clang-format on

  std::stringstream ss;
  ss << "message=" << std::quoted(a);

  std::stringstream ss2;
  ss2 << " b=" << std::quoted(b);

  ASSERT_TRUE(s.find(ss.str()) != std::string::npos);
  ASSERT_TRUE(s.find(std::format("location=\"struclog_tests.cpp:{}\"", current_line)) != std::string::npos);
  ASSERT_TRUE(s.find(ss2.str()) != std::string::npos);
}

TEST(struclog, Escapes) {
  const std::string a = "test a great message";
  const std::string b = "test \"great\" name";
  const std::string c = "test 'great' name";
  const int num = 123;
  // clang-format off
  int current_line = __LINE__; auto s = printout(sl::Log{ std::string(a) }("b", std::string(b))("c", std::string(c))("num", num));
  // clang-format on

  std::stringstream ss;
  ss << "message=" << std::quoted(a) << " ";
  ss << std::format("location=\"struclog_tests.cpp:{}\"", current_line) << " ";
  ss << "b=" << std::quoted(b) << " ";
  ss << "c=" << std::quoted(c) << " ";
  ss << "num=" << num;

  std::cout << ss.str() << "\n";

  ASSERT_EQ(s, ss.str());
}