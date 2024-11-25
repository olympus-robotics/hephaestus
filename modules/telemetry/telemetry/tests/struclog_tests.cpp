//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
#include <atomic>
#include <format>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "hephaestus/telemetry/struclog.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace hephtelemetry::tests {

namespace ht = heph::telemetry;

namespace {
auto printout(const ht::LogEntry& s) -> std::string {
  return ht::format(s);
}
}  // namespace

TEST(struclog, LogEntry) {
  const std::string a = "test a great message";
  const std::string b = "test \"great\" name";
  // clang-format off
  const int current_line = __LINE__; auto log_entry = ht::LogEntry{heph::LogLevel::INFO, ht::MessageWithLocation{a.c_str()}} << ht::Field{.key="b",.value="test \"great\" name"};
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
  EXPECT_TRUE(s.find(std::format("location=\"struclog_tests.cpp:{}\"", current_line)) != std::string::npos);
}

TEST(struclog, Escapes) {
  const std::string a = "test a great message";
  const std::string c = "test 'great' name";
  // clang-format off
  const int current_line = __LINE__; auto log_entry = ht::LogEntry{heph::LogLevel::INFO, ht::MessageWithLocation{a.c_str()}} << ht::Field{.key="c", .value=c};
  // clang-format on
  auto s = printout(log_entry);
  ht::internal::log(std::move(log_entry));
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
    ss << std::format("location=\"struclog_tests.cpp:{}\"", current_line);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "c=" << std::quoted(c);
    EXPECT_TRUE(s.find(ss.str()) != std::string::npos);
  }
}

/// @brief Custom sink that will write the log to a string.
class MockLogSink final : public ht::ILogSink {
public:
  void send(const ht::LogEntry& s) override {
    log_ = format(s);
    flag_.test_and_set();
    flag_.notify_all();
  }

  [[nodiscard]] auto getLog() const -> const std::string& {
    return log_;
  }

  void wait() const {
    flag_.wait(false);
  }

private:
  std::string log_;
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

TEST(struclog, sink) {
  const int num = 123;

  auto log_entry = ht::LogEntry{ heph::LogLevel::ERROR, "test another great message" }
                   << ht::Field{ .key = "num", .value = num };
  auto mock_sink = std::make_unique<MockLogSink>();
  const MockLogSink* sink_ptr = mock_sink.get();
  ht::registerLogSink(std::move(mock_sink));
  ht::internal::log(std::move(log_entry));
  sink_ptr->wait();
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(sink_ptr->getLog().find(ss.str()) != std::string::npos);
  }
}

TEST(struclog, log) {
  auto mock_sink = std::make_unique<MockLogSink>();
  const MockLogSink* sink_ptr = mock_sink.get();
  ht::registerLogSink(std::move(mock_sink));
  heph::log(heph::LogLevel::ERROR, "test another great message");
  sink_ptr->wait();

  EXPECT_TRUE(sink_ptr->getLog().find("message=\"test another great message\"") != std::string::npos);
}

TEST(struclog, logWithFields) {
  const int num = 123;

  auto mock_sink = std::make_unique<MockLogSink>();
  const MockLogSink* sink_ptr = mock_sink.get();
  ht::registerLogSink(std::move(mock_sink));
  heph::log(heph::LogLevel::ERROR, "test another great message", "num", num, "test", "lala");
  sink_ptr->wait();
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(sink_ptr->getLog().find(ss.str()) != std::string::npos);
  }
  {
    std::stringstream ss;
    ss << "test=\"lala\"";
    EXPECT_TRUE(sink_ptr->getLog().find(ss.str()) != std::string::npos);
  }
}

}  // namespace hephtelemetry::tests
