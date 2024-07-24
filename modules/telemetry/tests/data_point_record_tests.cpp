//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/random/random_container.h"
#include "hephaestus/random/random_generator.h"
#include "hephaestus/random/random_type.h"
#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/data_point_record.h"
#include "hephaestus/telemetry/data_point_sink.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::telemetry::tests {
namespace {

class MockMeasureSink final : public IDataPointSink {
public:
  void send(const DataPoint& data_point) override {
    measure_entries_.push_back(data_point);
    flag_.test_and_set();
    flag_.notify_all();
  }

  [[nodiscard]] auto getMeasureEntries() const -> const std::vector<DataPoint>& {
    return measure_entries_;
  }

  void wait() const {
    flag_.wait(false);
  }

private:
  std::vector<DataPoint> measure_entries_;
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

struct NestedNestedObject {
  [[nodiscard]] auto operator==(const NestedNestedObject&) const -> bool = default;
  std::vector<int64_t> vector;
  bool boolean{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedNestedObject, vector, boolean);

struct NestedObject {
  [[nodiscard]] auto operator==(const NestedObject&) const -> bool = default;
  std::vector<int64_t> vector;
  double float64{};
  NestedNestedObject nested;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedObject, vector, float64, nested);

struct Dummy {
  [[nodiscard]] auto operator==(const Dummy&) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Dummy {
    return {
      .boolean = random::randomT<bool>(mt),
      .int32 = random::randomT<int32_t>(mt),
      .int64 = random::randomT<int64_t>(mt),
      .uint32 = random::randomT<uint32_t>(mt),
      .uint64 = random::randomT<uint64_t>(mt),
      .float32 = random::randomT<float>(mt),
      .float64 = random::randomT<double>(mt),
      .string = random::randomT<std::string>(mt),
      .nested = { .vector = random::randomT<std::vector<int64_t>>(mt),
                  .float64 = random::randomT<double>(mt),
                  .nested = { .vector = random::randomT<std::vector<int64_t>>(mt),
                              .boolean = random::randomT<bool>(mt) } },
    };
  }

  bool boolean{};
  int32_t int32{};
  int64_t int64{};
  uint32_t uint32{};
  uint64_t uint64{};
  float float32{};
  double float64{};
  std::string string;
  NestedObject nested;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Dummy, boolean, int32, int64, uint32, uint64, float32, float64, string,
                                   nested);

}  // namespace

TEST(Measure, DataPoint) {
  auto mt = random::createRNG();

  auto mock_sink = std::make_unique<MockMeasureSink>();
  const auto* mock_sink_ptr = mock_sink.get();
  registerDataPointSink(std::move(mock_sink));

  const auto entry =
      DataPoint{ .component = random::randomT<std::string>(mt),
                 .tag = random::randomT<std::string>(mt),
                 .timestamp = random::randomT<ClockT::time_point>(mt),
                 .values = { { random::randomT<std::string>(mt), random::randomT<int64_t>(mt) } } };
  record(entry);

  mock_sink_ptr->wait();
  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(1));
  EXPECT_EQ(entry, measure_entries.front());
}

TEST(Measure, Serialization) {
  {
    Dummy d;
    serdes::deserializeFromJSON(serdes::serializeToJSON(d), d);
  }

  static constexpr auto COMPONENT = "component";
  static constexpr auto TAG = "tag";

  auto mt = random::createRNG();

  auto mock_sink = std::make_unique<MockMeasureSink>();
  const auto* mock_sink_ptr = mock_sink.get();
  registerDataPointSink(std::move(mock_sink));

  auto dummy = Dummy::random(mt);
  record(COMPONENT, TAG, dummy);

  mock_sink_ptr->wait();
  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(1));
  const auto entry = measure_entries.front();

  EXPECT_EQ(entry.component, COMPONENT);
  EXPECT_EQ(entry.tag, TAG);
  EXPECT_GE(entry.timestamp.time_since_epoch().count(), 0);
  const std::unordered_map<std::string, DataPoint::ValueType> expected_values{
    { "boolean", dummy.boolean },
    { "int32", static_cast<int64_t>(dummy.int32) },
    { "int64", dummy.int64 },
    { "uint32", static_cast<int64_t>(dummy.uint32) },
    { "uint64", static_cast<int64_t>(dummy.uint64) },
    { "float32", static_cast<double>(dummy.float32) },
    { "float64", dummy.float64 },
    { "string", dummy.string },
    { "nested.float64", dummy.nested.float64 },
    { "nested.nested.boolean", dummy.nested.nested.boolean },
  };
  EXPECT_EQ(entry.values, expected_values);
}
}  // namespace heph::telemetry::tests
