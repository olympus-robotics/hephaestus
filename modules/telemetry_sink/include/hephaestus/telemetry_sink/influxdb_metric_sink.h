//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstddef>
#include <memory>
#include <string>

#include "hephaestus/telemetry/metric_sink.h"

namespace influxdb {
class InfluxDB;
}

namespace heph::telemetry_sink {

struct InfluxDBSinkConfig {
  std::string url;
  std::string token;
  std::string database;
  std::size_t batch_size{ 0 };
};

class InfluxDBSink final : public telemetry::IMetricSink {
public:
  ~InfluxDBSink() override = default;
  [[nodiscard]] static auto create(InfluxDBSinkConfig config) -> std::unique_ptr<InfluxDBSink>;

  void send(const telemetry::Metric& entry) override;

private:
  explicit InfluxDBSink(InfluxDBSinkConfig config);

private:
  InfluxDBSinkConfig config_;
  std::unique_ptr<influxdb::InfluxDB> influxdb_;
};

}  // namespace heph::telemetry_sink
