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

#include <exec/task.hpp>
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
struct ReceivingOperationData {
  bool triggered{ false };
  bool executed{ false };
};

struct ReceivingOperation : Node<ReceivingOperation, ReceivingOperationData> {
  static constexpr auto NAME = "ReceivingOperation";

  static auto trigger(ReceivingOperation& operation) {
    EXPECT_FALSE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().triggered = true;
    return stdexec::just();
  }

  static void execute(ReceivingOperation& operation) {
    EXPECT_TRUE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().executed = true;
    operation.engine().requestStop();
  }
};

TEST(NodeTests, nodeBasic) {
  NodeEngineConfig config;
  config.prefix = "test";
  NodeEngine engine{ config };
  auto dummy = engine.createNode<ReceivingOperation>();

  engine.run();
  EXPECT_TRUE(dummy->data().triggered);
  EXPECT_TRUE(dummy->data().executed);
  EXPECT_FALSE(ReceivingOperation::HAS_PERIOD);
  EXPECT_EQ(dummy->nodeName(), "/test/ReceivingOperation");
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

struct TriggerExceptionOperation : Node<TriggerExceptionOperation, ReceivingOperationData> {
  static auto trigger(TriggerExceptionOperation& operation) {
    EXPECT_FALSE(operation.data().triggered);
    EXPECT_FALSE(operation.data().executed);
    operation.data().triggered = true;
    panic("Running around with scissors is dangerous");
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

  EXPECT_THROW(engine.run(), Panic);
  EXPECT_TRUE(dummy->data().triggered);
  EXPECT_FALSE(dummy->data().executed);
  EXPECT_FALSE(TriggerExceptionOperation::HAS_PERIOD);
}

struct ExecutionExceptionOperation : Node<ExecutionExceptionOperation, ReceivingOperationData> {
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
    panic("Running around with scissors is dangerous");
  }
};

TEST(NodeTests, nodeExecutionException) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<ExecutionExceptionOperation>();

  EXPECT_THROW(engine.run(), Panic);
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
    ++operation.data().period_called;
    return PERIOD;
  }

  static void execute(PeriodicOperation& operation) {
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
  NodeEngine engine{
    { .context_config = { .io_ring_config = {},
                          .timer_options = { heph::concurrency::io_ring::ClockMode::SIMULATED } },
      .prefix = "",
      .endpoints = {} }
  };
  auto dummy = engine.createNode<PeriodicOperation>();

  engine.run();
  EXPECT_GE(dummy->data().period_called, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
  EXPECT_GE(dummy->data().executed, PeriodicOperation::RUNTIME / PeriodicOperation::PERIOD);
}

struct PeriodicMissingDeadlineOperation : Node<PeriodicMissingDeadlineOperation, PeriodicOperationData> {
  static constexpr auto PERIOD = std::chrono::milliseconds{ 50 };
  static constexpr auto RUNTIME = std::chrono::milliseconds{ 299 };

  static void execute(PeriodicMissingDeadlineOperation& operation) {
    if (operation.engine().elapsed() > RUNTIME) {
      operation.engine().requestStop();
      return;
    }
    ++operation.data().executed;
    std::this_thread::sleep_for(PERIOD * 2);
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
  EXPECT_EQ(dummy->data().executed - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), 1);
  EXPECT_TRUE(PeriodicMissingDeadlineOperation::HAS_PERIOD);
}

TEST(NodeTests, nodePeriodicMissingDeadlineSimulated) {
  NodeEngine engine{
    { .context_config = { .io_ring_config = {},
                          .timer_options = { heph::concurrency::io_ring::ClockMode::SIMULATED } },
      .prefix = "",
      .endpoints = {} }
  };
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  auto dummy = engine.createNode<PeriodicMissingDeadlineOperation>();

  engine.run();
  EXPECT_LE(dummy->data().executed - 1,
            PeriodicMissingDeadlineOperation::RUNTIME / (PeriodicMissingDeadlineOperation::PERIOD * 2));
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), 1);
  EXPECT_TRUE(PeriodicMissingDeadlineOperation::HAS_PERIOD);
}

struct CoroutineOperation : Node<CoroutineOperation> {
  static auto trigger(CoroutineOperation* self) -> exec::task<void> {
    co_await stdexec::just();
    self->triggered = true;
  }

  static auto execute(CoroutineOperation* self) -> exec::task<void> {
    self->engine().requestStop();
    co_await stdexec::just();
    self->executed = true;
  }

  bool triggered{ false };
  bool executed{ false };
};

TEST(NodeTests, coroutineTrigger) {
  NodeEngine engine{ {} };

  auto node = engine.createNode<CoroutineOperation>();
  engine.run();
  EXPECT_TRUE(node->triggered);
  EXPECT_TRUE(node->executed);
}
}  // namespace heph::conduit::tests
