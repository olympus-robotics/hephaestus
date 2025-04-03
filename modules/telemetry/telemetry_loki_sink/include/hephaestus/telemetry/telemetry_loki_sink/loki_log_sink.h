//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <map>
#include <string>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>
#include <cpr/session.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry {

struct LokiLogSinkConfig {
  static constexpr auto DEFAULT_LOKI_PORT = 3100;
  static constexpr auto DEFAULT_LOKI_HOST = "localhost";
  static constexpr std::chrono::duration<double> DEFAULT_FLUSH_PERIOD{ 0.2 };
  uint32_t loki_port = DEFAULT_LOKI_PORT;
  std::string loki_host = DEFAULT_LOKI_HOST;
  LogLevel log_level = LogLevel::TRACE;
  std::string domain;  /// Used to group logs from different binaries.
  std::chrono::duration<double> flush_period = DEFAULT_FLUSH_PERIOD;
};

/// @brief A simple sink that passes the logs to ABSL.
///        Per default entry is formatted via heph::telemetry::format.
class LokiLogSink final : public ILogSink {
public:
  explicit LokiLogSink(const LokiLogSinkConfig& config);
  ~LokiLogSink() override;

  void send(const LogEntry& entry) override;

private:
  void spinOnce();

private:
  static constexpr auto LOKI_URL_FORMAT = "http://{}:{}/loki/api/v1/push";
  LogLevel min_log_level_;
  cpr::Session cpr_session_;

  std::map<std::string, std::string> stream_labels_;

  absl::Mutex mutex_;
  std::unordered_map<LogLevel, std::vector<LogEntry>> log_entries_ ABSL_GUARDED_BY(mutex_);

  concurrency::Spinner spinner_;
};

}  // namespace heph::telemetry
