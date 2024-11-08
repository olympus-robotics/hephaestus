//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

#include <utility>

#include <absl/log/absl_log.h>

#include "hephaestus/telemetry/struclog.h"

namespace heph::telemetry {

AbslLogSink::AbslLogSink() : formatter_([](const LogEntry& l) { return format(l); }) {
}

AbslLogSink::AbslLogSink(Formatter&& f) : formatter_(std::move(f)) {
}

void AbslLogSink::send(const LogEntry& entry) {
  // Use the ABSL_LOG version since it is more explicit than LOG
  // Use NoPrefix since all information is contained in the entry and we want logfmt formatting.
  switch (entry.level) {
    case Level::TRACE:
      ABSL_VLOG(2).NoPrefix() << formatter_(entry);
      break;
    case Level::DEBUG:
      ABSL_VLOG(1).NoPrefix() << formatter_(entry);
      break;
    case Level::INFO:
      ABSL_LOG(INFO).NoPrefix() << formatter_(entry);
      break;
    case Level::WARN:
      ABSL_LOG(WARNING).NoPrefix() << formatter_(entry);
      break;
    case Level::ERROR:
      ABSL_LOG(ERROR).NoPrefix() << formatter_(entry);
      break;
    case Level::FATAL:
      ABSL_LOG(FATAL).NoPrefix() << formatter_(entry);
      break;
  }
}
}  // namespace heph::telemetry