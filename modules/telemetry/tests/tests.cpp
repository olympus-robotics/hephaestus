//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <format>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
// #include <utility>

#include <gtest/gtest.h>

#include "hephaestus/telemetry/struclog.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace hephtelemetry::tests {

namespace ht = heph::telemetry;
// NOLINTBEGIN google-build-using-namespace
using namespace heph::telemetry::literals;
// NOLINTEND google-build-using-namespace

auto printout(const ht::LogEntry& s) -> std::string {
  return ht::format(s);
}

TEST(struclog, LogEntry) {
  const std::string a = "test a great message";
  const std::string b = "test \"great\" name";
  // clang-format off
  // int current_line = __LINE__; auto s = printout(ht::LogEntry{ht::Level::Info, std::string(a)} | "b"_f("test \"great\" name"));
  const int current_line = __LINE__; auto log_entry = ht::LogEntry{ht::Level::Info, std::string(a)} | ht::Field{"b","test \"great\" name"};
  // Old format
  // int current_line = __LINE__; auto s = printout(ht::LogEntry{std::string(a)}("b", std::string(b)));
  // clang-format on
  auto s = printout(log_entry);
  {
    std::stringstream ss;
    ss << "message=" << std::quoted(a);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << " b=" << std::quoted(b);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  EXPECT_TRUE(s.find(std::format("location=\"tests.cpp:{}\"", current_line)) != std::string::npos);
}

TEST(struclog, Escapes) {
  const std::string a = "test a great message";
  const std::string b = "test \"great\" name";
  const std::string c = "test 'great' name";
  const int num = 123;
  // clang-format off
  const int current_line = __LINE__; auto log_entry = ht::LogEntry{ht::Level::Info, a} | "b"_f(b) | ht::Field{"c", c} | "num"_f(num);
  // Old format:
  // int current_line = __LINE__; auto s = printout(ht::LogEntry{ std::string(a) }("b", std::string(b))( "c", std::string(c) )( "num", num));
  // clang-format on
  auto s = printout(log_entry);
  ht::log(log_entry);
  {
    std::stringstream ss;
    ss << "level=info";
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "message=" << std::quoted(a);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << std::format("location=\"tests.cpp:{}\"", current_line);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "b=" << std::quoted(b);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "c=" << std::quoted(c);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
}

/** @brief Custom sink that will write the log to a string. Want to test that but I can not bring ABSL to flush at the right time.
 */
struct TestSink : public ht::ILogSink {
  explicit TestSink(std::shared_ptr<std::string> ref) : output{ ref } {
  }

  ~TestSink() override = default;

  void send(const LogEntry& s) override {
    *output = format(s);
    std::cout << "output sink: " << *output << "\n" << std::flush;
  }

  int num;

  std::shared_ptr<std::string> output;
};

TEST(struclog, sink) {
  ht::initLog();
  std::shared_ptr<std::string> output;
  const std::string a = "test another great message";
  const int num = 123;
  // clang-format off
  const int current_line = __LINE__; const auto log_entry = ht::LogEntry{ht::Level::Error, a} | "num"_f(num);
  // clang-format on
  ht::registerLogSink(std::make_unique<TestSink>(output));
  ht::log(log_entry);
  std::cout << current_line << "\n";
  std::cout << "output: " << output << "\n" << std::flush;
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(output.find(ss.str()) != std::string::npos);
  }
}

TEST(Example, Example) {
  EXPECT_TRUE(true);
}
}  // namespace hephtelemetry::tests
