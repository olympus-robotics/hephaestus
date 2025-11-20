//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/base.h>
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/telemetry/log/scope.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::telemetry::tests {

namespace ht = heph::telemetry;

/// @brief Custom sink that will write the log to a string.
class MockLogSink final : public ht::ILogSink {
public:
  void send(const ht::LogEntry& s) override {
    std::ignore = logs_.tryEmplace(fmt::format("|{}|", s));
  }

  [[nodiscard]] auto getLog() -> std::string {
    auto log = logs_.waitAndPop();
    if (log.has_value()) {
      return std::move(log.value());
    }

    return "";
  }

  auto empty() const -> bool {
    return logs_.empty();
  }

private:
  containers::BlockingQueue<std::string> logs_{ 1 };
};

class LogTestFixture : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    auto mock_sink = std::make_unique<MockLogSink>();
    sink_ptr = mock_sink.get();
    heph::telemetry::registerLogSink(std::move(mock_sink));
  }

  static MockLogSink* sink_ptr;
};

MockLogSink* LogTestFixture::sink_ptr{ nullptr };

TEST_F(LogTestFixture, LogEntry) {
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

TEST_F(LogTestFixture, Escapes) {
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

TEST_F(LogTestFixture, sink) {
  const int num = 123;

  heph::log(heph::INFO, "test another great message", "num", num);
  const auto log = sink_ptr->getLog();
  {
    std::stringstream ss;
    ss << "num=" << num;
    EXPECT_TRUE(log.find(ss.str()) != std::string::npos);
  }
}

TEST_F(LogTestFixture, log) {
  using namespace std::literals::string_literals;

  heph::log(heph::INFO, "test another great message");

  EXPECT_TRUE(sink_ptr->getLog().find("message=\"test another great message\"") != std::string::npos);
}

TEST_F(LogTestFixture, logString) {
  using namespace std::literals::string_literals;

  heph::log(heph::INFO, "as string"s);

  EXPECT_TRUE(sink_ptr->getLog().find("message=\"as string\"") != std::string::npos);
}

TEST_F(LogTestFixture, logLibFmt) {
  const int num = 456;

  heph::log(heph::INFO, fmt::format("this {} is formatted", num));

  EXPECT_TRUE(sink_ptr->getLog().find("message=\"this 456 is formatted\"") != std::string::npos);
}

// TODO(mfehr): Currently not supported by the libc++ version we have.
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE >= 13
TEST_F(LogTestFixture, logStdFmt) {
  const int num = 456;
  heph::log(heph::INFO, std::format("this {} is formatted", num));

  EXPECT_TRUE(sink_ptr->getLog().find("message=\"this 456 is formatted\"") != std::string::npos);
}
#endif

TEST_F(LogTestFixture, logWithFields) {
  using namespace std::literals::string_literals;

  const int num = 123;

  heph::log(heph::INFO, "test another great message", "num", num, "test", "lala");

  const auto log = sink_ptr->getLog();
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

TEST_F(LogTestFixture, logIfWithFields) {
  using namespace std::literals::string_literals;

  const int num = 123;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  heph::logIf(heph::INFO, num == 123, "test another great message", "num", num, "test", "lala");

  const auto log = sink_ptr->getLog();
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

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  heph::logIf(heph::INFO, num == 124, "test another great message", "num", num, "test", "lala");
  EXPECT_TRUE(sink_ptr->empty());
}

TEST_F(LogTestFixture, LogWithScope) {
  using namespace std::literals::string_literals;

  heph::log(heph::INFO, "a message in global scope");
  EXPECT_THAT(sink_ptr->getLog(), testing::HasSubstr("module=global"));

  const Scope scope1("robot1", "module1");
  heph::log(heph::INFO, "a message in module1 scope");
  EXPECT_THAT(sink_ptr->getLog(), testing::HasSubstr("module=/robot1/module1"));

  {
    const Scope scope2("robot1", "module2");
    heph::log(heph::INFO, "a message in module1/module2 scope");
    EXPECT_THAT(sink_ptr->getLog(), testing::HasSubstr("module=/robot1/module2"));
  }

  heph::log(heph::INFO, "another message in module1 scope");
  EXPECT_THAT(sink_ptr->getLog(), testing::HasSubstr("module=/robot1/module1"));
}
}  // namespace heph::telemetry::tests
