//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>

#include <fmt/core.h>

#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {

class TerminalSink final : public ITelemetrySink {
public:
  ~TerminalSink() override = default;

  void send(const LogEntry& log_entry) override;
};

void TerminalSink::send(const LogEntry& log_entry) {
  // TODO: we can replace this with spdlog or absl log
  fmt::println("Component: {} | Tag: {} | Values: [\n{}\n]", log_entry.component, log_entry.tag,
               log_entry.json_values);
}

auto createTerminalSink() -> std::unique_ptr<ITelemetrySink> {
  return std::make_unique<TerminalSink>();
}

}  // namespace heph::telemetry
