//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/metrics/detail/struct_to_flatmap.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::telemetry::tests {
namespace {

class MockMetricSink final : public IMetricSink {
public:
  void send(const Metric& metric) override {
    measure_entries_.push_back(metric);
    flag_.test_and_set();
    flag_.notify_all();
  }

  [[nodiscard]] auto getMeasureEntries() const -> const std::vector<Metric>& {
    return measure_entries_;
  }

  void wait() const {
    flag_.wait(false);
  }

private:
  std::vector<Metric> measure_entries_;
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

struct NestedNestedObject {
  [[nodiscard]] auto operator==(const NestedNestedObject&) const -> bool = default;
  bool boolean{};
};

struct NestedObject {
  [[nodiscard]] auto operator==(const NestedObject&) const -> bool = default;
  double float64{};
  NestedNestedObject nested;
};

struct Dummy {
  [[nodiscard]] auto operator==(const Dummy&) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Dummy {
    return {
      .boolean = random::random<bool>(mt),
      .int32 = random::random<int32_t>(mt),
      .int64 = random::random<int64_t>(mt),
      .uint32 = random::random<uint32_t>(mt),
      .uint64 = random::random<uint64_t>(mt),
      .float32 = random::random<float>(mt),
      .float64 = random::random<double>(mt),
      .string = "k" + random::random<std::string>(mt),
      .nested = { .float64 = random::random<double>(mt), .nested = { .boolean = random::random<bool>(mt) } },
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
}  // namespace

TEST(StructToFlatmap, StructToFlatmap) {
  auto mt = random::createRNG();

  const auto dummy = Dummy::random(mt);
  const auto values = detail::structToKeyValuePairs(dummy);
  const std::vector<Metric::KeyValueType> expected_values{
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
  EXPECT_EQ(values, expected_values);
}

TEST(Measure, Metric) {
  auto mt = random::createRNG();

  auto mock_sink = std::make_unique<MockMetricSink>();
  const auto* mock_sink_ptr = mock_sink.get();
  registerMetricSink(std::move(mock_sink));

  const auto entry =
      Metric{ .component = random::random<std::string>(mt),
              .tag = random::random<std::string>(mt),
              .timestamp = random::random<ClockT::time_point>(mt),
              .values = { { "k" + random::random<std::string>(mt), random::random<int64_t>(mt) } } };
  record(entry);

  mock_sink_ptr->wait();
  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(1));
  EXPECT_EQ(entry, measure_entries.front());
}

TEST(Measure, Serialization) {
  static constexpr auto COMPONENT = "component";
  static constexpr auto TAG = "tag";

  auto mt = random::createRNG();

  auto mock_sink = std::make_unique<MockMetricSink>();
  const auto* mock_sink_ptr = mock_sink.get();
  registerMetricSink(std::move(mock_sink));

  auto dummy = Dummy::random(mt);
  record(COMPONENT, TAG, dummy);

  mock_sink_ptr->wait();
  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(1));
  const auto entry = measure_entries.front();

  EXPECT_EQ(entry.component, COMPONENT);
  EXPECT_EQ(entry.tag, TAG);
  EXPECT_GE(entry.timestamp.time_since_epoch().count(), 0);
  const std::vector<Metric::KeyValueType> expected_values{
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
