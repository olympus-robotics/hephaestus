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

#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/metrics/detail/struct_to_key_value_pairs.h"
#include "hephaestus/telemetry/metrics/metric_builder.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"
#include "hephaestus/test_utils/heph_test.h"
#include "hephaestus/utils/timing/mock_clock.h"

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

  void wait() {
    flag_.wait(false);
    flag_.clear();
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

struct StructToFlatmapTest : heph::test_utils::HephTest {};
struct MetricTest : heph::test_utils::HephTest {
  MetricTest() {
    auto mock_sink = std::make_unique<MockMetricSink>();
    mock_sink_ptr = mock_sink.get();
    registerMetricSink(std::move(mock_sink));
  }

  MockMetricSink* mock_sink_ptr{};
};

}  // namespace

TEST_F(StructToFlatmapTest, StructToFlatmap) {
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

TEST_F(MetricTest, Metric) {
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

TEST_F(MetricTest, Serialization) {
  static constexpr auto COMPONENT = "component";
  static constexpr auto TAG = "tag";

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

TEST_F(MetricTest, MetricBuilder) {
  static constexpr auto COMPONENT = "component";
  static constexpr auto TAG = "tag";
  static constexpr auto PERIOD = std::chrono::milliseconds{ 1 };
  const auto now = ClockT::now();
  {
    MetricBuilder builder(COMPONENT, TAG, now);
    builder.addValue("key1", "value_key", int64_t{ 0 });
    {
      auto timer = builder.measureScopeExecutionTime("key1", utils::timing::MockClock::now);
      utils::timing::MockClock::advance(PERIOD);
    }
  }
  mock_sink_ptr->wait();

  static constexpr auto OTHER_TAG = "other_tag";
  static constexpr auto OTHER_PERIOD = std::chrono::milliseconds{ 2 };
  const auto other_now = ClockT::now();
  {
    MetricBuilder builder(COMPONENT, OTHER_TAG, other_now);
    builder.addValue("key2", "value_key2", int64_t{ 1 });
    {
      auto timer = builder.measureScopeExecutionTime("key2", utils::timing::MockClock::now);
      utils::timing::MockClock::advance(OTHER_PERIOD);
    }
  }
  mock_sink_ptr->wait();

  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(2));

  const auto expected_metrics = std::vector<Metric>{{
                                      .component = COMPONENT,
                                      .tag = TAG,
                                      .timestamp = now,
                                      .values = {
                                        { "key1.value_key", int64_t{0} },
                                        { "key1.elapsed_s", 0.001},
                                      },
                                  },
                                  {
                                      .component = COMPONENT,
                                      .tag = OTHER_TAG,
                                      .timestamp = other_now,
                                      .values = {
                                        { "key2.value_key2", int64_t{1} },
                                        { "key2.elapsed_s", 0.002},
                                      },
                                  } };

  EXPECT_EQ(measure_entries, expected_metrics);
}
}  // namespace heph::telemetry::tests
