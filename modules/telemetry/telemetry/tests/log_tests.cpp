//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors

#include <chrono>
// #include <condition_variable>
#include <format>
#include <iomanip>
#include <iostream>
#include <memory>
// #include <mutex>
#include <source_location>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::telemetry::tests {

namespace ht = heph::telemetry;

/// @brief Custom sink that will write the log to a string.
class MockLogSink final : public ht::ILogSink {
  using Line = size_t;

public:
  void send(const ht::LogEntry& s) override {
    logs_.emplace(s.location.line(), fmt::format("{}", s));
  }

  [[nodiscard]] auto getLog(std::source_location loc) -> const std::string& {
    /*
    std::unique_lock<std::mutex> lock(mtx);
    std::cout << "waiting for " << loc.line() << "\n";
    cv.wait(lock, [&]() { return logs_.find(loc.line()) != logs_.cend(); });
*/
    using namespace std::chrono_literals;
    while (logs_.find(loc.line()) == logs_.cend()) {
      std::this_thread::sleep_for(10ms);
    }
    return logs_.at(loc.line());
  }

private:
  std::map<Line, std::string> logs_;
  // std::mutex mtx;
  // std::condition_variable cv;
};

class LogTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto mock_sink = std::make_unique<MockLogSink>();
    sink_ptr = mock_sink.get();
    heph::telemetry::registerLogSink(std::move(mock_sink));
  }

  MockLogSink* sink_ptr;
};

TEST_F(LogTest, LogEntry) {
  const std::string a = "test a great message";
  const std::string b = "test \"great\" name";
  // clang-format off
  const int current_line = __LINE__; auto log_entry = ht::LogEntry{heph::INFO, ht::MessageWithLocation{a.c_str()}} << ht::Field{.key="b",.value="test \"great\" name"};
  // clang-format on
  auto s = fmt::format("{}", log_entry);
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
  EXPECT_TRUE(s.find(fmt::format("location=\"log_tests.cpp:{}\"", current_line)) != std::string::npos);
}

TEST_F(LogTest, Escapes) {
  const std::string a = "test a great message";
  const std::string c = "test 'great' name";
  // clang-format off
  const int current_line = __LINE__; auto log_entry = ht::LogEntry{heph::INFO, ht::MessageWithLocation{a.c_str()}} << ht::Field{.key="c", .value=c};
  // clang-format on
  auto s = fmt::format("{}", log_entry);
  {
    std::stringstream ss;
    ss << "level=INFO";
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "message=" << std::quoted(a);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << fmt::format("location=\"log_tests.cpp:{}\"", current_line);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "c=" << std::quoted(c);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
}

TEST_F(LogTest, sink) {
  const int num = 123;

  // clang-format off
  const auto loc = std::source_location::current(); auto log_entry = ht::LogEntry{ heph::ERROR, "test another great message" } << ht::Field{ .key = "num", .value = num };
  // clang-format on

  ht::internal::log(std::move(log_entry));
  fmt::print("{}", sink_ptr->getLog(loc));
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(sink_ptr->getLog(loc).find(ss.str()) != std::string::npos);
  }
}
TEST_F(LogTest, log) {
  using namespace std::literals::string_literals;

  // clang-format off
   const auto loc = std::source_location::current(); heph::log(heph::ERROR, "test another great message");
  // clang-format on

  EXPECT_TRUE(sink_ptr->getLog(loc).find("message=\"test another great message\"") != std::string::npos);
}

TEST_F(LogTest, logString) {
  using namespace std::literals::string_literals;

  // clang-format off
   const auto loc = std::source_location::current(); heph::log(heph::ERROR, "as string"s);
  // clang-format on

  EXPECT_TRUE(sink_ptr->getLog(loc).find("message=\"as string\"") != std::string::npos);
}

TEST_F(LogTest, logLibFmt) {
  const int num = 456;
  // clang-format off
   const auto loc = std::source_location::current(); heph::log(heph::ERROR, fmt::format("this {} is formatted", num));
  // clang-format on

  EXPECT_TRUE(sink_ptr->getLog(loc).find("message=\"this 456 is formatted\"") != std::string::npos);
}

TEST_F(LogTest, logStdFmt) {
  const int num = 456;
  // clang-format off
   const auto loc = std::source_location::current(); heph::log(heph::ERROR, std::format("this {} is formatted", num));
  // clang-format on

  EXPECT_TRUE(sink_ptr->getLog(loc).find("message=\"this 456 is formatted\"") != std::string::npos);
}

TEST_F(LogTest, logWithFields) {
  using namespace std::literals::string_literals;

  const int num = 123;

  // clang-format off
   const auto loc = std::source_location::current(); heph::log(heph::ERROR, "test another great message",
 "num", num, "test", "lala");
  // clang-format on

  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(sink_ptr->getLog(loc).find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "test=\"lala\"";
    EXPECT_TRUE(sink_ptr->getLog(loc).find(ss.str()) != std::string::npos);
  }
}

}  // namespace heph::telemetry::tests
