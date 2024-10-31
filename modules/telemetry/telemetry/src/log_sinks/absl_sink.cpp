//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

#include <absl/log/log.h>

namespace heph::telemetry {

AbslLogSink::AbslLogSink() : formatter([](const LogEntry& l) { return format(l); }) {
}

AbslLogSink::AbslLogSink(Formatter&& f) : formatter(std::move(f)) {
}

void AbslLogSink::send(const LogEntry& entry) {
  // Use the ABSL_LOG version since it is more explicit than LOG
  // Use NoPrefix since all information is contained in the entry and we want logfmt formatting.
  switch (entry.level) {
    case Level::Trace:
      ABSL_VLOG(2).NoPrefix() << formatter_(entry);
      break;
    case Level::Debug:
      ABSL_VLOG(1).NoPrefix() << formatter_(entry);
      ;
      break;
    case Level::Info:
      ABSL_LOG(INFO).NoPrefix() << formatter_(entry);
      ;
      break;
    case Level::Warn:
      ABSL_LOG(WARNING).NoPrefix() << formatter_(entry);
      ;
      break;
    case Level::Error:
      ABSL_LOG(ERROR).NoPrefix() << formatter_(entry);
      ;
      break;
    case Level::Fatal:
      ABSL_LOG(FATAL).NoPrefix() << formatter_(entry);
      ;
      break;
  }
}
}  // namespace heph::telemetry