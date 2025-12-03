//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <string>
#include <utility>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int argc, const char* argv[]) -> int {
  using namespace std::literals::string_literals;
  const heph::utils::StackTrace stack;

  auto desc = heph::cli::ProgramDescription("Log example");
  desc.defineFlag("debug", 'd', "enable debug log level").defineFlag("trace", 't', "enable trace log level");
  const auto args = std::move(desc).parse(argc, argv);

  auto log_level = heph::INFO;
  if (args.getOption<bool>("debug")) {
    log_level = heph::DEBUG;
  }
  if (args.getOption<bool>("trace")) {
    log_level = heph::TRACE;
  }
  heph::telemetry::makeAndRegisterLogSink<heph::telemetry::AbslLogSink>(log_level);

  heph::log(heph::WARN, "testing absl log sink"s);

  const int num = 1234;
  heph::log(heph::INFO, "absl log sink with fields", "num", num, "quoted_string", "test");

  heph::log(heph::DEBUG, "debug absl log sink with fields", "num", num);

  heph::log(heph::TRACE, "debug absl log sink with fields", "num", num);

  return 0;
}
