//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/log_sinks/absl_sink.h"

#include <utility>

#include <absl/log/absl_log.h>

#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry {

AbslLogSink::AbslLogSink() : formatter_([](const LogEntry& l) { return format(l); }) {
}

AbslLogSink::AbslLogSink(ILogSink::Formatter&& f) : formatter_(std::move(f)) {
}

void AbslLogSink::send(const LogEntry& entry) {
  // Use the ABSL_LOG version since it is more explicit than LOG
  // Use NoPrefix since all information is contained in the entry and we want logfmt formatting.
  switch (entry.level) {
    case LogLevel::TRACE:
      ABSL_VLOG(2).NoPrefix() << formatter_(entry);
      break;
    case LogLevel::DEBUG:
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
      break;
  }
}
}  // namespace heph::telemetry
