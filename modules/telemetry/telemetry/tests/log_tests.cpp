//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors

#include <format>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::telemetry::tests {

namespace ht = heph::telemetry;

/// @brief Custom sink that will write the log to a string.
class MockLogSink final : public ht::ILogSink {
public:
  void send(const ht::LogEntry& s) override {
    std::ignore = logs_.tryEmplace(fmt::format("{}", s));
  }

  [[nodiscard]] auto getLog() -> std::string {
    auto log = logs_.waitAndPop();
    if (log.has_value()) {
      return std::move(log.value());
    }

    return "";
  }

private:
  containers::BlockingQueue<std::string> logs_{ std::nullopt };
};

class LogTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto mock_sink = std::make_unique<MockLogSink>();
    sink_ptr_ = mock_sink.get();
    heph::telemetry::registerLogSink(std::move(mock_sink));
  }

  MockLogSink* sink_ptr_;
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

  heph::log(heph::ERROR, "test another great message", "num", num);
  const auto log = sink_ptr_->getLog();
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(log.find(ss.str()) != std::string::npos);
  }
}

TEST_F(LogTest, log) {
  using namespace std::literals::string_literals;

  heph::log(heph::ERROR, "test another great message");

  EXPECT_TRUE(sink_ptr_->getLog().find("message=\"test another great message\"") != std::string::npos);
}

TEST_F(LogTest, logString) {
  using namespace std::literals::string_literals;

  heph::log(heph::ERROR, "as string"s);

  EXPECT_TRUE(sink_ptr_->getLog().find("message=\"as string\"") != std::string::npos);
}

TEST_F(LogTest, logLibFmt) {
  const int num = 456;

  heph::log(heph::ERROR, fmt::format("this {} is formatted", num));

  EXPECT_TRUE(sink_ptr_->getLog().find("message=\"this 456 is formatted\"") != std::string::npos);
}

TEST_F(LogTest, logStdFmt) {
  const int num = 456;
  heph::log(heph::ERROR, std::format("this {} is formatted", num));

  EXPECT_TRUE(sink_ptr_->getLog().find("message=\"this 456 is formatted\"") != std::string::npos);
}

TEST_F(LogTest, logWithFields) {
  using namespace std::literals::string_literals;

  const int num = 123;

  heph::log(heph::ERROR, "test another great message", "num", num, "test", "lala");

  const auto log = sink_ptr_->getLog();
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(log.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "test=\"lala\"";
    EXPECT_TRUE(log.find(ss.str()) != std::string::npos);
  }
}

}  // namespace heph::telemetry::tests
