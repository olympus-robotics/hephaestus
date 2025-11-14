//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/loki_sink/loki_log_sink.h"

#include <chrono>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <absl/synchronization/mutex.h>
#include <cpr/cprtypes.h>
#include <cpr/session.h>
#include <cpr/status_codes.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <magic_enum.hpp>
#include <rfl/json/write.hpp>
#include <unistd.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/utils/utils.h"

namespace heph::telemetry {
namespace {

using Value = std::vector<std::variant<std::string, std::map<std::string, std::string>>>;

struct Stream {
  std::map<std::string, std::string> stream;
  std::vector<Value> values;
};

/// The struct of the Json request to Loki
/// The specification can be found here:
/// https://grafana.com/docs/loki/latest/reference/loki-http-api/#ingest-logs
struct PushRequest {
  std::vector<Stream> streams;
};

[[nodiscard]] auto createServiceNameFromBinaryName() -> std::string {
  auto binary_name = utils::getBinaryPath();
  panicIf(!binary_name.has_value(), "cannot get binary name");
  return binary_name->filename();  // NOLINT(bugprone-unchecked-optional-access);
}

[[nodiscard]] auto formatMessage(const LogEntry& entry) -> std::string {
  return fmt::format("{} | {}", entry.message, fmt::join(entry.fields, " "));
}

[[nodiscard]] auto createValue(const LogEntry& entry) -> Value {
  Value value;

  value.emplace_back(std::to_string(
      std::chrono::duration_cast<std::chrono::nanoseconds>(entry.time.time_since_epoch()).count()));
  value.emplace_back(formatMessage(entry));

  // Add metadata
  value.emplace_back(std::map<std::string, std::string>{
      { "location", fmt::format("{}:{}", entry.location.file_name(), entry.location.line()) },
      { "thread_id", fmt::format("{}", fmt::streamed(entry.thread_id)) } });

  return value;
}

[[nodiscard]] auto toStream(LogLevel level, const std::string& module, const std::vector<LogEntry>& entries,
                            const std::map<std::string, std::string>& stream_labels) -> Stream {
  Stream stream;
  stream.stream = stream_labels;
  stream.stream["level"] = magic_enum::enum_name(level);
  stream.stream["module"] = module;

  stream.values.reserve(entries.size());
  for (const auto& entry : entries) {
    stream.values.push_back(createValue(entry));
  }

  return stream;
}

[[nodiscard]] auto createPushRequest(const LokiLogSink::LogEntryPerLevel& entries,
                                     const std::map<std::string, std::string>& stream_labels) -> PushRequest {
  PushRequest request;
  for (const auto& [level, module_logs] : entries) {
    for (const auto& [module, logs] : module_logs) {
      request.streams.push_back(toStream(level, module, logs, stream_labels));
    }
  }

  return request;
}

[[nodiscard]] auto createStaticStreamLabels(const LokiLogSinkConfig& config)
    -> std::map<std::string, std::string> {
  return std::map<std::string, std::string>{ { "domain", config.domain },
                                             { "service_name", createServiceNameFromBinaryName() },
                                             { "pid", std::to_string(getpid()) },
                                             { "hostname", utils::getHostName() } };
}

}  // namespace

LokiLogSink::LokiLogSink(const LokiLogSinkConfig& config)
  : min_log_level_(config.log_level)
  , stream_labels_(createStaticStreamLabels(config))
  , spinner_(
        [this]() {
          spinOnce();
          return concurrency::Spinner::SpinResult::CONTINUE;
        },
        config.flush_period) {
  cpr_session_.SetUrl(fmt::format(LOKI_URL_FORMAT, config.loki_host, config.loki_port));
  cpr_session_.SetHeader(cpr::Header{ { "Content-Type", "application/json" } });
  spinner_.start();
}

LokiLogSink::~LokiLogSink() {
  spinner_.stop().get();
}

void LokiLogSink::spinOnce() {
  LogEntryPerLevel entries;
  {
    const absl::MutexLock lock{ &mutex_ };
    if (log_entries_.empty()) {
      return;
    }

    entries = std::move(log_entries_);
    log_entries_ = {};
  }

  const auto request = createPushRequest(entries, stream_labels_);
  // NOTE: we use JSON serialization to send data to Loki, but Loki also supports Protobuf with snappy if
  // performance become a problem.
  auto json_str = rfl::json::write(request);

  cpr_session_.SetBody(cpr::Body{ std::move(json_str) });
  const auto response = cpr_session_.Post();
  if (response.status_code > cpr::status::HTTP_MULTIPLE_CHOICE) {
    fmt::println(stderr, "failed to send log to Loki, status code: {}, content\n{}", response.status_code,
                 json_str);
  }
}

void LokiLogSink::send(const LogEntry& entry) {
  if (entry.level < min_log_level_) {
    return;
  }

  const absl::MutexLock lock{ &mutex_ };
  log_entries_[entry.level][entry.module].push_back(entry);
}

}  // namespace heph::telemetry
