//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>
#include <utility>

#include <absl/log/log.h>
#include <cpr/cpr.h>

#include "hephaestus/serdes/serdes.h"
#include "hephaestus/telemetry/sink.h"
#include "proto_conversion.h"  // NOLINT(misc-include-cleaner)

namespace heph::telemetry {

class RESTSink final : public ITelemetrySink {
public:
  explicit RESTSink(RESTSinkConfig config);
  ~RESTSink() override = default;

  void send(const MetricEntry& log_entry) override;

private:
  RESTSinkConfig config_;
};

RESTSink::RESTSink(RESTSinkConfig config) : config_(std::move(config)) {
}

void RESTSink::send(const MetricEntry& log_entry) {
  auto log_entry_json = serdes::serializeToJSON(log_entry);
  auto response = cpr::Post(cpr::Url{ config_.url }, cpr::Body{ std::move(log_entry_json) },
                            cpr::Header{ { "Content-Type", "application/json" } });

  LOG_IF(ERROR, response.status_code != 200)
      << fmt::format("Failed to publish to REST endpoint with code {}, reason: {}, message {}",
                     response.status_code, response.reason, response.text);
}

auto createRESTSink(RESTSinkConfig config) -> std::unique_ptr<ITelemetrySink> {
  return std::make_unique<RESTSink>(std::move(config));
}

}  // namespace heph::telemetry
