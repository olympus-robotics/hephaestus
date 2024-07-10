//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>
#include <utility>

#include <cpr/cpr.h>

#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {

class RESTSink final : public ITelemetrySink {
public:
  explicit RESTSink(RESTSinkConfig config);
  ~RESTSink() override = default;

  void send(const LogEntry& log_entry) override;

private:
  RESTSinkConfig config_;
};

RESTSink::RESTSink(RESTSinkConfig config) : config_(std::move(config)) {
  (void)config_;
}

void RESTSink::send(const LogEntry& log_entry) {
  (void)log_entry;
}

auto createRESTSink(RESTSinkConfig config) -> std::unique_ptr<ITelemetrySink> {
  return std::make_unique<RESTSink>(std::move(config));
}

}  // namespace heph::telemetry
