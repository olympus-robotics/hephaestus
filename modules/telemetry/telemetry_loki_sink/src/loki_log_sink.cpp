//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/telemetry_loki_sink/loki_log_sink.h"

#include <fmt/format.h>
#include <rfl/json.hpp>  // NOLINT(misc-include-cleaner)
#include <unistd.h>

#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/exception.h"
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
  throwExceptionIf<InvalidParameterException>(!binary_name.has_value(), "cannot get binary name");
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

[[nodiscard]] auto toStream(const LogEntry& entry, const std::map<std::string, std::string>& stream_labels)
    -> Stream {
  Stream stream;
  stream.stream = stream_labels;
  stream.stream["level"] = magic_enum::enum_name(entry.level);

  stream.values.push_back(createValue(entry));
  return stream;
}

[[nodiscard]] auto createPushRequest(const LogEntry& entry,
                                     const std::map<std::string, std::string>& stream_labels) -> PushRequest {
  PushRequest request;
  request.streams.push_back(toStream(entry, stream_labels));

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
  : min_log_level_(config.log_level), stream_labels_(createStaticStreamLabels(config)) {
  cpr_session_.SetUrl(fmt::format(LOKI_URL_FORMAT, config.loki_host, config.loki_port));
  cpr_session_.SetHeader(cpr::Header{ { "Content-Type", "application/json" } });
}

void LokiLogSink::send(const LogEntry& entry) {
  if (entry.level < min_log_level_) {
    return;
  }

  // TODO(@fbrizzi): extend to support batching
  const auto request = createPushRequest(entry, stream_labels_);
  auto json_str = rfl::json::write(request);

  cpr_session_.SetBody(cpr::Body{ std::move(json_str) });
  const auto response = cpr_session_.Post();
  if (response.status_code > cpr::status::HTTP_MULTIPLE_CHOICE) {
    fmt::println(stderr, "failed to send log to Loki, status code: {}", response.status_code);
  }
}

}  // namespace heph::telemetry
