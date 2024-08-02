//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "hephaestus/telemetry/struclog.h"
#include "hephaestus/utils/stack_trace.h"

namespace ht = heph::telemetry;
// NOLINTBEGIN google-build-using-namespace
using namespace heph::telemetry::literals;
// NOLINTEND google-build-using-namespace

struct TestSink : public ht::ILogSink {
  explicit TestSink(std::string* ref) : output{ ref } {
  }

  ~TestSink() override = default;

  void send(std::string_view s) override {
    *output = s;
    std::cout << "output sink: " << *output << "\n" << std::flush;
  }

  std::string* output;
};

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  const heph::utils::StackTrace stack;

  ht::initLog();
  std::string output;
  {
    const std::string a = "test another great message";
    const int num = 123;
    const auto log_entry = ht::LogEntry{ ht::Level::Warn, a } | "num"_f(num);
    ht::registerLogSink(std::make_unique<TestSink>(&output));
    ht::log(log_entry);
    ht::flushLog();
  }
  std::cout << "output: " << output << "\n" << std::flush;
}
