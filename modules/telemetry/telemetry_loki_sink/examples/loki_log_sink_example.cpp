//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>
#include <string>
#include <utility>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/telemetry_loki_sink/loki_log_sink.h"
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
  log_level = heph::TRACE;

  heph::telemetry::LokiLogSinkConfig config;
  config.log_level = log_level;
  config.label = "forkify";
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::LokiLogSink>(config));

  heph::log(heph::WARN, "testing loki log sink"s);

  const int num = 1234;
  heph::log(heph::INFO, "loki log sink with fields", "num", num, "quoted_string", "test");

  heph::log(heph::DEBUG, "debug loki debug log sink with fields", "num", num);

  heph::log(heph::TRACE, "debug loki trace log sink with fields", "num", num);

  heph::log(heph::ERROR, "debug loki error log sink with fields", "num", num);

  return 0;
}
