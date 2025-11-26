//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <memory>
#include <string>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/error_handling/panic_as_exception_scope.h"
#include "hephaestus/error_handling/panic_exception.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/log_sink.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::error_handling::tests {
namespace {

TEST(Panic, Exception) {
  const error_handling::PanicAsExceptionScope panic_scope;
  EXPECT_THROW(panic("This is a panic with value {}", 42), PanicException);
}

TEST(PanicIf, NoSideEffectWhenConditionFalse) {
  bool side_effect = false;
  HEPH_PANIC_IF(false, "{}", (side_effect = true));
  EXPECT_FALSE(side_effect);
}

TEST(PanicIf, SideEffectWhenConditionTrue) {
  const error_handling::PanicAsExceptionScope panic_scope;

  bool side_effect = false;

  try {
    HEPH_PANIC_IF(true, "{}", (side_effect = true));
  } catch (...) {  // NOLINT(bugprone-empty-catch)
  }

  EXPECT_TRUE(side_effect);
}

struct StringLogSink : public telemetry::ILogSink {
  void send(const telemetry::LogEntry& entry) override {
    std::string location_field;

    for (const auto& field : entry.fields) {
      if (field.key == "location") {
        location_field = field.value;
      }
    }

    last_log_message = fmt::format("{} {}", location_field, entry.message);
  }

  std::string last_log_message;
};

TEST(PanicIf, Output) {
  const error_handling::PanicAsExceptionScope panic_scope;

  auto string_log_sink = std::make_unique<StringLogSink>();
  auto* const string_log_sink_ptr = string_log_sink.get();

  telemetry::registerLogSink(std::move(string_log_sink));

  heph::telemetry::flushLogEntries();
  EXPECT_EQ(string_log_sink_ptr->last_log_message, "");

  try {
    HEPH_PANIC_IF(true, "{}", 42);
  } catch (...) {  // NOLINT(bugprone-empty-catch)
  }

  heph::telemetry::flushLogEntries();
  EXPECT_EQ(string_log_sink_ptr->last_log_message,
            "\"modules/error_handling/tests/panic_tests.cpp:75\" program terminated with panic");

  telemetry::removeAllLogSinks();
}

}  // namespace
}  // namespace heph::error_handling::tests
