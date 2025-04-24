//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/log_sinks/absl_sink.h"

#include <utility>

#include <absl/log/absl_log.h>
#include <absl/log/globals.h>
#include <fmt/format.h>

#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry {

AbslLogSink::AbslLogSink(LogLevel log_level)
  : formatter_([](const LogEntry& l) { return fmt::format("{}", l); }) {
  switch (log_level) {
    case LogLevel::TRACE:
      absl::SetGlobalVLogLevel(2);
      break;
    case LogLevel::DEBUG:
      absl::SetGlobalVLogLevel(1);
      break;
    case LogLevel::INFO:
    case LogLevel::WARN:
    case LogLevel::ERROR:
    case LogLevel::FATAL:
      absl::SetGlobalVLogLevel(0);
      break;
  }
}

AbslLogSink::AbslLogSink(ILogSink::Formatter&& f) : formatter_(std::move(f)) {
}

void AbslLogSink::send(const LogEntry& entry) {
  // Use the ABSL_LOG version since it is more explicit than LOG
  // Use NoPrefix since all information is contained in the entry and we want logfmt formatting.
  switch (entry.level) {
    case LogLevel::TRACE:
      // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
      ABSL_VLOG(2).NoPrefix() << formatter_(entry);
      break;
    case LogLevel::DEBUG:
      // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
      ABSL_VLOG(1).NoPrefix() << formatter_(entry);
      break;
    case LogLevel::INFO:
      ABSL_LOG(INFO).NoPrefix() << formatter_(entry);
      break;
    case LogLevel::WARN:
      ABSL_LOG(WARNING).NoPrefix() << formatter_(entry);
      break;
    case LogLevel::ERROR:
      ABSL_LOG(ERROR).NoPrefix() << formatter_(entry);
      break;
    case LogLevel::FATAL:
      ABSL_LOG(FATAL).NoPrefix() << formatter_(entry);
      if (entry.stack_trace.has_value()) {
        ABSL_LOG(FATAL).NoPrefix() << entry.stack_trace.value();
      }
      break;
    default:
      ABSL_LOG(FATAL).NoPrefix() << "LogLevel not implemented.";
  }
}
}  // namespace heph::telemetry
