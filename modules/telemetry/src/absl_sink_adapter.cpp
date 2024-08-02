//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/absl_sink_adapter.h"

#include <absl/log/log_sink_registry.h>

namespace heph::telemetry {
LogSinkAdapter::LogSinkAdapter(std::unique_ptr<ILogSink> sink) : sink_{ std::move(sink) } {
  absl::AddLogSink(this);
}

~LogSinkAdapter() {
  absl::RemoveLogSink(this);
}

void Send(const absl::LogEntry& entry) {
  sink_->send(entry.text_message());
}
}  // namespace heph::telemetry