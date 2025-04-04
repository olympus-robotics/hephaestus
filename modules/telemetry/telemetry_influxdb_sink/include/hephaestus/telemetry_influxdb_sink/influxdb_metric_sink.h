//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/telemetry/metric_sink.h"

namespace influxdb {
class InfluxDB;
}

namespace heph::telemetry_sink {

struct InfluxDBSinkConfig {
  std::string url;
  std::string token;
  std::string database;
  std::optional<std::size_t> batch_size{ std::nullopt };  //! If specified the sink will batch this many
                                                          //! points before sending them.
  std::optional<std::chrono::duration<double>> flush_period{
    std::nullopt
  };  //! If specified the sink will flush the batch at this
      //! period. NOTE: setting this will invalidate the
      //! batch_size.
};

class InfluxDBSink final : public telemetry::IMetricSink {
public:
  ~InfluxDBSink() override;
  [[nodiscard]] static auto create(InfluxDBSinkConfig config) -> std::unique_ptr<InfluxDBSink>;

  void send(const telemetry::Metric& entry) override;

private:
  explicit InfluxDBSink(InfluxDBSinkConfig config);

private:
  InfluxDBSinkConfig config_;
  std::mutex mutex_;
  std::unique_ptr<influxdb::InfluxDB> influxdb_;
  std::unique_ptr<concurrency::Spinner> spinner_;
};

}  // namespace heph::telemetry_sink
