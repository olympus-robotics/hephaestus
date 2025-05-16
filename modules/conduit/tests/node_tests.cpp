//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

#include <fmt/base.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/io_ring/timer.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/exception.h"

namespace heph::conduit::tests {
struct BasicOperation : Node<BasicOperation> {
  auto trigger(NodeEngine& /*engine*/) {
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(executed);
    triggered = true;
    return stdexec::just();
  }

  void operator()(NodeEngine& engine) {
    EXPECT_TRUE(triggered);
    EXPECT_FALSE(executed);
    executed = true;
    engine.requestStop();
  }

  bool triggered{ false };
  bool executed{ false };
};

TEST(NodeTests, nodeBasic) {
  NodeEngine engine{ {} };
  BasicOperation dummy;

  engine.addNode(dummy);

  engine.run();
  EXPECT_TRUE(dummy.triggered);
  EXPECT_TRUE(dummy.executed);
  EXPECT_FALSE(BasicOperation::HAS_PERIOD);
}

struct RepeatOperation : Node<RepeatOperation> {
  static constexpr unsigned NUM_REPEATS = 10;
  auto trigger() {
    EXPECT_EQ(triggered, executed);
    ++triggered;
    return stdexec::just();
  }

  void operator()(NodeEngine& engine) {
    EXPECT_EQ(triggered - 1, executed);
    ++executed;
    if (executed == NUM_REPEATS) {
      engine.requestStop();
    }
  }

  unsigned triggered{ 0 };
  unsigned executed{ 0 };
};

TEST(NodeTests, nodeRepeat) {
  NodeEngine engine{ {} };
  RepeatOperation dummy;

  engine.addNode(dummy);

  engine.run();
  EXPECT_EQ(dummy.triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_EQ(dummy.triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_FALSE(RepeatOperation::HAS_PERIOD);
}

struct RepeatPoolOperation : Node<RepeatPoolOperation> {
  static constexpr unsigned NUM_REPEATS = 10;
  auto trigger() {
    EXPECT_EQ(triggered, executed);
    ++triggered;
    return stdexec::just();
  }

  auto operator()(NodeEngine& engine) {
    EXPECT_EQ(triggered - 1, executed);
    if (!context_thread.has_value()) {
      context_thread.emplace(std::this_thread::get_id());
    } else {
      EXPECT_EQ(*context_thread, std::this_thread::get_id());
    }
    return stdexec::schedule(engine.poolScheduler()) | stdexec::then([&] {
             ++executed;
             EXPECT_NE(*context_thread, std::this_thread::get_id());
             EXPECT_NE(*context_thread, parent_thread);
             if (executed == NUM_REPEATS) {
               engine.requestStop();
             }
           });
  }

  unsigned triggered{ 0 };
  unsigned executed{ 0 };
  std::optional<std::thread::id> context_thread;
  std::thread::id parent_thread;
};

TEST(NodeTests, nodePoolRepeat) {
  RepeatPoolOperation dummy;
  dummy.parent_thread = std::this_thread::get_id();

  std::thread runner([&] {
    NodeEngine engine{ {} };
    engine.addNode(dummy);
    engine.run();
  });
  runner.join();
  EXPECT_EQ(dummy.triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_EQ(dummy.triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_FALSE(RepeatOperation::HAS_PERIOD);
}

struct TriggerExceptionOperation : Node<TriggerExceptionOperation> {
  auto trigger() {
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(executed);
    triggered = true;
    heph::panic("Running around with scissors is dangerous");
    return stdexec::just();
  }

  void operator()() {
    EXPECT_TRUE(triggered);
    EXPECT_FALSE(executed);
    executed = true;
  }

  bool triggered{ false };
  bool executed{ false };
};

TEST(NodeTests, nodeTriggerException) {
  NodeEngine engine{ {} };
  TriggerExceptionOperation dummy;

  engine.addNode(dummy);

  EXPECT_THROW(engine.run(), heph::Panic);
  EXPECT_TRUE(dummy.triggered);
  EXPECT_FALSE(dummy.executed);
  EXPECT_FALSE(TriggerExceptionOperation::HAS_PERIOD);
}

struct ExecutionExceptionOperation : Node<ExecutionExceptionOperation> {
  auto trigger() {
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(executed);
    triggered = true;
    return stdexec::just();
  }

  void operator()() {
    EXPECT_TRUE(triggered);
    EXPECT_FALSE(executed);
    executed = true;
    heph::panic("Running around with scissors is dangerous");
  }

  bool triggered{ false };
  bool executed{ false };
};

TEST(NodeTests, nodeExecutionException) {
  NodeEngine engine{ {} };
  ExecutionExceptionOperation dummy;

  engine.addNode(dummy);

  EXPECT_THROW(engine.run(), heph::Panic);
  EXPECT_TRUE(dummy.triggered);
  EXPECT_TRUE(dummy.executed);
  EXPECT_FALSE(ExecutionExceptionOperation::HAS_PERIOD);
}

struct PeriodicOperation : Node<PeriodicOperation> {
  static constexpr auto PERIOD = std::chrono::milliseconds{ 50 };
  static constexpr auto RUNTIME = std::chrono::milliseconds{ 300 };
  auto period() {
    EXPECT_EQ(period_called, executed);
    ++period_called;
    return PERIOD;
  }

  void operator()(NodeEngine& engine) {
    EXPECT_EQ(period_called - 1, executed);
    ++executed;
    // fmt::println(stderr, "{}", engine.elapsed());
    if (engine.elapsed() > RUNTIME) {
      engine.requestStop();
    }
  }

  unsigned period_called{ 0 };
  unsigned executed{ 0 };
};

TEST(NodeTests, nodePeriodic) {
  NodeEngine engine{ {} };
  PeriodicOperation dummy;

  engine.addNode(dummy);

  engine.run();
  EXPECT_GE(dummy.period_called, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_GE(dummy.executed, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_TRUE(PeriodicOperation::HAS_PERIOD);
}

TEST(NodeTests, nodePeriodicSimulated) {
  NodeEngine engine{ { .context_config = {
                           .io_ring_config = {},
                           .timer_options = { heph::concurrency::io_ring::ClockMode::SIMULATED } } } };
  PeriodicOperation dummy;

  engine.addNode(dummy);

  engine.run();
  EXPECT_GE(dummy.period_called, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_GE(dummy.executed, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_TRUE(PeriodicOperation::HAS_PERIOD);
}

struct PeriodicMissingDeadlineOperation : Node<PeriodicMissingDeadlineOperation> {
  static constexpr auto PERIOD = std::chrono::milliseconds{ 50 };
  static constexpr auto RUNTIME = std::chrono::milliseconds{ 300 };
  auto period() {
    EXPECT_EQ(period_called, executed);
    ++period_called;
    return PERIOD;
  }

  void operator()(NodeEngine& engine) {
    EXPECT_EQ(period_called - 1, executed);
    ++executed;
    std::this_thread::sleep_for(PERIOD * 2);
    if (engine.elapsed() > RUNTIME) {
      engine.requestStop();
    }
  }

  unsigned period_called{ 0 };
  unsigned executed{ 0 };
};

namespace ht = heph::telemetry;
class MockLogSink final : public ht::ILogSink {
public:
  void send(const ht::LogEntry& s) override {
    if (s.level == heph::WARN && s.message == "Missed deadline") {
      ++num_messages;
      fmt::println(stderr, "{}", fmt::join(s.fields, ", "));
    }
  }

  std::atomic<unsigned> num_messages{ 0 };
};

TEST(NodeTests, nodePeriodicMissingDeadline) {
  NodeEngine engine{ {} };
  PeriodicMissingDeadlineOperation dummy;
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  engine.addNode(dummy);

  engine.run();
  EXPECT_EQ(dummy.period_called - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  EXPECT_EQ(dummy.executed - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), dummy.executed - 1);
  EXPECT_TRUE(PeriodicMissingDeadlineOperation::HAS_PERIOD);
}

TEST(NodeTests, nodePeriodicMissingDeadlineSimulated) {
  NodeEngine engine{ { .context_config = {
                           .io_ring_config = {},
                           .timer_options = { heph::concurrency::io_ring::ClockMode::SIMULATED } } } };
  PeriodicMissingDeadlineOperation dummy;
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  engine.addNode(dummy);

  engine.run();

  EXPECT_EQ(dummy.period_called - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  EXPECT_EQ(dummy.executed - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), dummy.executed - 1);
  EXPECT_TRUE(PeriodicMissingDeadlineOperation::HAS_PERIOD);
}
}  // namespace heph::conduit::tests
