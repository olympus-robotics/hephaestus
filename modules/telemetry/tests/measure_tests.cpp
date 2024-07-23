//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/random/random_container.h"
#include "hephaestus/random/random_generator.h"
#include "hephaestus/random/random_type.h"
#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/measure.h"
#include "hephaestus/telemetry/measure_sink.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::telemetry::tests {
namespace {

class MockMeasureSink final : public IMeasureSink {
public:
  void send(const MeasureEntry& measure_entry) override {
    measure_entries_.push_back(measure_entry);
    flag_.test_and_set();
    flag_.notify_all();
  }

  [[nodiscard]] auto getMeasureEntries() const -> const std::vector<MeasureEntry>& {
    return measure_entries_;
  }

  void wait() const {
    flag_.wait(false);
  }

private:
  std::vector<MeasureEntry> measure_entries_;
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

struct Dummy {
  [[nodiscard]] auto operator==(const Dummy& other) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Dummy {
    return {
      .counter = random::randomT<std::size_t>(mt),
      .error = random::randomT<double>(mt),
      .message = random::randomT<std::string>(mt),
    };
  }

  std::size_t counter{};
  double error{};
  std::string message;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Dummy, counter, error, message);

}  // namespace

TEST(Measure, MeasureEntry) {
  auto mt = random::createRNG();

  auto mock_sink = std::make_unique<MockMeasureSink>();
  const auto* mock_sink_ptr = mock_sink.get();
  registerSink(std::move(mock_sink));

  const auto entry = MeasureEntry{ .component = random::randomT<std::string>(mt),
                                   .tag = random::randomT<std::string>(mt),
                                   .measure_timestamp = random::randomT<ClockT::time_point>(mt),
                                   .json_values = random::randomT<std::string>(mt) };
  measure(entry);

  mock_sink_ptr->wait();
  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(1));
  EXPECT_EQ(entry, measure_entries.front());
}

TEST(Measure, Serialization) {
  static constexpr auto COMPONENT = "component";
  static constexpr auto TAG = "tag";

  auto mt = random::createRNG();

  auto mock_sink = std::make_unique<MockMeasureSink>();
  const auto* mock_sink_ptr = mock_sink.get();
  registerSink(std::move(mock_sink));

  auto dummy = Dummy::random(mt);
  measure(COMPONENT, TAG, dummy);

  mock_sink_ptr->wait();
  const auto& measure_entries = mock_sink_ptr->getMeasureEntries();
  EXPECT_THAT(measure_entries, SizeIs(1));
  const auto entry = measure_entries.front();

  EXPECT_EQ(entry.component, COMPONENT);
  EXPECT_EQ(entry.tag, TAG);
  EXPECT_GE(entry.measure_timestamp.time_since_epoch().count(), 0);
  Dummy dummy_des;
  serdes::deserializeFromJSON(entry.json_values, dummy_des);
  EXPECT_EQ(dummy_des, dummy);
}
}  // namespace heph::telemetry::tests
