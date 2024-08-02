//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <absl/log/log_sink.h>

#include "hephaestus/telemetry/struclog.h"

namespace heph::telemetry {

/**
 * @brief An adapter to connect ILogSink to absl.
 *        This may be used as an addition to AbslLogSink, so you can add the Custom sinks after ABSL
 *        Objectives for this might be that you have special performance/flushing reuqirements that ABSL
 * fullfills or for compatibility Note that you have to manage this class, since the sinks get unregistered
 * from absl on destruction of the adapter.
 *
 */
class LogSinkAdapter final : public absl::LogSink {
public:
  explicit LogSinkAdapter(std::unique_ptr<ILogSink> sink);

  ~LogSinkAdapter() override;

  LogSinkAdapter(const LogSinkAdapter&) = delete;                     // Delete copy constructor
  LogSinkAdapter(LogSinkAdapter&&) = default;                         // Default move assignment operator
  auto operator=(const LogSinkAdapter&) -> LogSinkAdapter& = delete;  // Delete copy assignment operator
  auto operator=(LogSinkAdapter&&) -> LogSinkAdapter& = default;      // Default move assignment operator

  void Send(const absl::LogEntry& entry) override;

private:
  std::unique_ptr<ILogSink> sink_;
};
}  // namespace heph::telemetry