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

struct LokiPushRequest {
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

[[nodiscard]] auto toStream(const LogEntry& entry, const std::map<std::string, std::string>& stream_labels)
    -> Stream {
  Stream stream;
  stream.stream = stream_labels;
  stream.stream["level"] = magic_enum::enum_name(entry.level);

  Value value;
  value.emplace_back(std::to_string(
      std::chrono::duration_cast<std::chrono::nanoseconds>(entry.time.time_since_epoch()).count()));
  value.emplace_back(formatMessage(entry));
  value.emplace_back(std::map<std::string, std::string>{
      { "location", fmt::format("{}:{}", entry.location.file_name(), entry.location.line()) },
      { "thread_id", fmt::format("{}", fmt::streamed(entry.thread_id)) } });

  stream.values.push_back(std::move(value));
  return stream;
}

[[nodiscard]] auto toLokiPushRequest(const LogEntry& entry,
                                     const std::map<std::string, std::string>& stream_labels)
    -> LokiPushRequest {
  LokiPushRequest request;
  request.streams.push_back(toStream(entry, stream_labels));

  return request;
}

}  // namespace

LokiLogSink::LokiLogSink(const LokiLogSinkConfig& config)
  : min_log_level_(config.log_level)
  , loki_url_(fmt::format(LOKI_URL_FORMAT, config.loki_host, config.loki_port))
  , post_request_header_(cpr::Header{ { "Content-Type", "application/json" } })
  , stream_labels_(std::map<std::string, std::string>{ { "domain", config.label },
                                                       { "service_name", createServiceNameFromBinaryName() },
                                                       { "pid", std::to_string(getpid()) },
                                                       { "hostname", utils::getHostName() } }) {
}

void LokiLogSink::send(const LogEntry& entry) {
  if (entry.level < min_log_level_) {
    return;
  }

  // TODO(@fbrizzi): extend to support batching
  auto request = toLokiPushRequest(entry, stream_labels_);
  auto json_str = rfl::json::write(request);

  auto response = cpr::Post(loki_url_, cpr::Body{ std::move(json_str) }, post_request_header_);

  if (response.status_code > cpr::status::HTTP_MULTIPLE_CHOICE) {
    fmt::println(stderr, "failed to send log to Loki, status code: {}", response.status_code);
  }
}

}  // namespace heph::telemetry
