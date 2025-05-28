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
struct BasicOperationData {
  bool triggered{ false };
  bool executed{ false };
};

struct BasicOperation : Node<BasicOperation, BasicOperationData> {
  static auto trigger(BasicOperation& operation) {
    EXPECT_FALSE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().triggered = true;
    return stdexec::just();
  }

  static void execute(BasicOperation& operation) {
    EXPECT_TRUE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().executed = true;
    operation.engine().requestStop();
  }
};

TEST(NodeTests, nodeBasic) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<BasicOperation>();

  engine.run();
  EXPECT_TRUE(dummy->data().triggered);
  EXPECT_TRUE(dummy->data().executed);
  EXPECT_FALSE(BasicOperation::HAS_PERIOD);
}

struct RepeatOperationData {
  unsigned triggered{ 0 };
  unsigned executed{ 0 };
};

struct RepeatOperation : Node<RepeatOperation, RepeatOperationData> {
  static constexpr unsigned NUM_REPEATS = 10;
  static auto trigger(RepeatOperation& operation) {
    EXPECT_EQ(operation.data().triggered, operation.data().executed);
    ++operation.data().triggered;
    return stdexec::just();
  }

  static void execute(RepeatOperation& operation) {
    EXPECT_EQ(operation.data().triggered - 1, operation.data().executed);
    ++operation.data().executed;
    if (operation.data().executed == NUM_REPEATS) {
      operation.engine().requestStop();
    }
  }
};

TEST(NodeTests, nodeRepeat) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<RepeatOperation>();

  engine.run();
  EXPECT_EQ(dummy->data().triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_EQ(dummy->data().triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_FALSE(RepeatOperation::HAS_PERIOD);
}

struct RepeatPoolOperationData {
  unsigned triggered{ 0 };
  unsigned executed{ 0 };
  std::optional<std::thread::id> context_thread;
  std::thread::id parent_thread;
};

struct RepeatPoolOperation : Node<RepeatPoolOperation, RepeatPoolOperationData> {
  static constexpr unsigned NUM_REPEATS = 10;
  static auto trigger(RepeatPoolOperation& operation) {
    EXPECT_EQ(operation.data().triggered, operation.data().executed);
    ++operation.data().triggered;
    return stdexec::just();
  }

  static auto execute(RepeatPoolOperation& operation) {
    EXPECT_EQ(operation.data().triggered - 1, operation.data().executed);
    if (!operation.data().context_thread.has_value()) {
      operation.data().context_thread.emplace(std::this_thread::get_id());
    } else {
      // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
      EXPECT_EQ(*operation.data().context_thread, std::this_thread::get_id());
    }
    return stdexec::schedule(operation.engine().poolScheduler()) | stdexec::then([&] {
             ++operation.data().executed;
             EXPECT_NE(*operation.data().context_thread, std::this_thread::get_id());
             EXPECT_NE(*operation.data().context_thread, operation.data().parent_thread);
             if (operation.data().executed == NUM_REPEATS) {
               operation.engine().requestStop();
             }
           });
  }

  unsigned triggered{ 0 };
  unsigned executed{ 0 };
  std::optional<std::thread::id> context_thread;
  std::thread::id parent_thread;
};

TEST(NodeTests, nodePoolRepeat) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<RepeatPoolOperation>();
  dummy->data().parent_thread = std::this_thread::get_id();

  std::thread runner([&] { engine.run(); });
  runner.join();
  EXPECT_EQ(dummy->data().triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_EQ(dummy->data().triggered, RepeatOperation::NUM_REPEATS);
  EXPECT_FALSE(RepeatOperation::HAS_PERIOD);
}

struct TriggerExceptionOperation : Node<TriggerExceptionOperation, BasicOperationData> {
  static auto trigger(TriggerExceptionOperation& operation) {
    EXPECT_FALSE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().triggered = true;
    heph::panic("Running around with scissors is dangerous");
    return stdexec::just();
  }

  static void execute(TriggerExceptionOperation& operation) {
    EXPECT_TRUE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().executed = true;
  }
};

TEST(NodeTests, nodeTriggerException) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<TriggerExceptionOperation>();

  EXPECT_THROW(engine.run(), heph::Panic);
  EXPECT_TRUE(dummy->data().triggered);
  EXPECT_FALSE(dummy->data().executed);
  EXPECT_FALSE(TriggerExceptionOperation::HAS_PERIOD);
}

struct ExecutionExceptionOperation : Node<ExecutionExceptionOperation, BasicOperationData> {
  static auto trigger(ExecutionExceptionOperation& operation) {
    EXPECT_FALSE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().triggered = true;
    return stdexec::just();
  }

  static void execute(ExecutionExceptionOperation& operation) {
    EXPECT_TRUE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().executed = true;
    heph::panic("Running around with scissors is dangerous");
  }
};

TEST(NodeTests, nodeExecutionException) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<ExecutionExceptionOperation>();

  EXPECT_THROW(engine.run(), heph::Panic);
  EXPECT_TRUE(dummy->data().triggered);
  EXPECT_TRUE(dummy->data().executed);
  EXPECT_FALSE(ExecutionExceptionOperation::HAS_PERIOD);
}

struct PeriodicOperationData {
  unsigned period_called{ 0 };
  unsigned executed{ 0 };
};

struct PeriodicOperation : Node<PeriodicOperation, PeriodicOperationData> {
  static constexpr auto PERIOD = std::chrono::milliseconds{ 50 };
  static constexpr auto RUNTIME = std::chrono::milliseconds{ 300 };
  static auto period(PeriodicOperation& operation) {
    EXPECT_EQ(operation.data().period_called, operation.data().executed);
    ++operation.data().period_called;
    return PERIOD;
  }

  static void execute(PeriodicOperation& operation) {
    EXPECT_EQ(operation.data().period_called - 1, operation.data().executed);
    ++operation.data().executed;
    if (operation.engine().elapsed() > RUNTIME) {
      operation.engine().requestStop();
    }
  }
};

TEST(NodeTests, nodePeriodic) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<PeriodicOperation>();

  engine.run();
  EXPECT_GE(dummy->data().period_called, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_GE(dummy->data().executed, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_TRUE(PeriodicOperation::HAS_PERIOD);
}

TEST(NodeTests, nodePeriodicSimulated) {
  NodeEngine engine{ { .context_config = {
                           .io_ring_config = {},
                           .timer_options = { heph::concurrency::io_ring::ClockMode::SIMULATED } } } };
  auto dummy = engine.createNode<PeriodicOperation>();

  engine.run();
  EXPECT_GE(dummy->data().period_called, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_GE(dummy->data().executed, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
}

struct PeriodicMissingDeadlineOperation : Node<PeriodicMissingDeadlineOperation, PeriodicOperationData> {
  static constexpr auto PERIOD = std::chrono::milliseconds{ 50 };
  static constexpr auto RUNTIME = std::chrono::milliseconds{ 300 };
  static auto period(PeriodicMissingDeadlineOperation& operation) {
    EXPECT_EQ(operation.data().period_called, operation.data().executed);
    ++operation.data().period_called;
    return PERIOD;
  }

  static void execute(PeriodicMissingDeadlineOperation& operation) {
    EXPECT_EQ(operation.data().period_called - 1, operation.data().executed);
    ++operation.data().executed;
    std::this_thread::sleep_for(PERIOD * 2);
    if (operation.engine().elapsed() > RUNTIME) {
      operation.engine().requestStop();
    }
  }
};

namespace ht = heph::telemetry;
class MockLogSink final : public ht::ILogSink {
public:
  void send(const ht::LogEntry& s) override {
    if (s.level == heph::WARN && s.message == PeriodicMissingDeadlineOperation::MISSED_DEADLINE_WARNING) {
      ++num_messages;
      fmt::println(stderr, "{}", fmt::join(s.fields, ", "));
    }
  }

  std::atomic<unsigned> num_messages{ 0 };
};

TEST(NodeTests, nodePeriodicMissingDeadline) {
  NodeEngine engine{ {} };
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  auto dummy = engine.createNode<PeriodicMissingDeadlineOperation>();

  engine.run();
  EXPECT_EQ(dummy->data().period_called - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  EXPECT_EQ(dummy->data().executed - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), dummy->data().executed - 1);
  EXPECT_TRUE(PeriodicMissingDeadlineOperation::HAS_PERIOD);
}

TEST(NodeTests, nodePeriodicMissingDeadlineSimulated) {
  NodeEngine engine{ { .context_config = {
                           .io_ring_config = {},
                           .timer_options = { heph::concurrency::io_ring::ClockMode::SIMULATED } } } };
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  auto dummy = engine.createNode<PeriodicMissingDeadlineOperation>();

  engine.run();
  EXPECT_EQ(dummy->data().period_called - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  EXPECT_EQ(dummy->data().executed - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), dummy->data().executed - 1);
  EXPECT_TRUE(PeriodicMissingDeadlineOperation::HAS_PERIOD);
}
}  // namespace heph::conduit::tests
