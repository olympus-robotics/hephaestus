//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cpr/cpr.h>

#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry {

struct LokiLogSinkConfig {
  static constexpr auto DEFAULT_LOKI_PORT = 3100;
  static constexpr auto DEFAULT_LOKI_HOST = "localhost";
  uint32_t loki_port = DEFAULT_LOKI_PORT;
  std::string loki_host = DEFAULT_LOKI_HOST;
  LogLevel log_level = LogLevel::TRACE;
  std::string domain;  /// Used to group logs from different binaries.
};

/// @brief A simple sink that passes the logs to ABSL.
///        Per default entry is formatted via heph::telemetry::format.
class LokiLogSink final : public ILogSink {
public:
  explicit LokiLogSink(const LokiLogSinkConfig& config);

  void send(const LogEntry& entry) override;

private:
  static constexpr auto LOKI_URL_FORMAT = "http://{}:{}/loki/api/v1/push";
  LogLevel min_log_level_;
  cpr::Session cpr_session_;

  std::map<std::string, std::string> stream_labels_;
};

}  // namespace heph::telemetry
