//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/when_any.hpp>
#include <fmt/base.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/io_ring/timer.h"
#include "hephaestus/conduit/accumulated_input.h"
#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/types_proto/numeric_value.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/string.h"

// NOLINTBEGIN(bugprone-unchecked-optional-access)

namespace heph::conduit::tests {
struct DummyOperation : Node<DummyOperation> {
  static auto trigger() {
    return stdexec::just();
  }
  static auto execute() {
  }
};

TEST(InputOutput, QueuedInputPolling) {
  DummyOperation dummy;
  QueuedInput<int, InputPolicy<1, RetrievalMethod::POLL>> input{ &dummy, "input" };

  std::optional<std::tuple<std::optional<int>>> res = stdexec::sync_wait(input.get());
  EXPECT_TRUE(res.has_value());
  EXPECT_FALSE(std::get<0>(*res).has_value());

  EXPECT_EQ(input.setValue(4711), InputState::OK);
  res = stdexec::sync_wait(input.get());
  EXPECT_TRUE(res.has_value());
  EXPECT_TRUE(std::get<0>(*res).has_value());
  EXPECT_EQ(*std::get<0>(*res), 4711);

  EXPECT_EQ(input.setValue(76), InputState::OK);
  EXPECT_EQ(input.setValue(9031), InputState::OVERFLOW);
  res = stdexec::sync_wait(input.get());
  EXPECT_TRUE(res.has_value());
  EXPECT_TRUE(std::get<0>(*res).has_value());
  EXPECT_EQ(*std::get<0>(*res), 76);
}

TEST(InputOutput, QueuedInputBlocking) {
  DummyOperation dummy;
  QueuedInput<int, InputPolicy<1, RetrievalMethod::BLOCK>> input{ &dummy, "input" };

  exec::async_scope scope;

  static constexpr int REFERENCE = 9485;
  int res = 0;

  scope.spawn(input.get() | stdexec::then([&](int value) { res = value; }));
  EXPECT_EQ(input.setValue(REFERENCE), InputState::OK);
  EXPECT_EQ(res, REFERENCE);

  EXPECT_EQ(input.setValue(REFERENCE + 1), InputState::OK);
  EXPECT_EQ(input.setValue(9031), InputState::OVERFLOW);
  scope.spawn(input.get() | stdexec::then([&](int value) { res = value; }));
  EXPECT_EQ(res, REFERENCE + 1);
}

struct OutputOperation : Node<OutputOperation> {
  static auto trigger() {
    return stdexec::just();
  }
  static constexpr int VALUE{ 4711 };
  static auto execute() -> int {
    return VALUE;
  }
};

struct InputOperatioData {
  bool called{ false };
};

struct InputOperation : Node<InputOperation, InputOperatioData> {
  QueuedInput<int> input1{ this, "input" };

  static auto trigger(InputOperation& self) {
    return self.input1.get();
  }

  static void execute(InputOperation& self, int input) {
    EXPECT_EQ(input, OutputOperation::VALUE);
    self.data().called = true;
    self.engine().requestStop();
  }
};

TEST(InputOutput, QueuedInputBasicInputOutput) {
  NodeEngine engine{ {} };

  auto out = engine.createNode<OutputOperation>();
  auto in = engine.createNode<InputOperation>();

  in->input1.connectTo(out);
  // connect(out, in.input1);
  engine.run();
  EXPECT_TRUE(in->data().called);
}

TEST(InputOutput, QueuedInputExplicitOutput) {
  NodeEngine engine{ {} };
  exec::async_scope scope;
  DummyOperation dummy;

  QueuedInput<std::string> input{ &dummy, "input" };
  Output<std::string> output{ &dummy, "output" };
  input.connectTo(output);
  scope.spawn(output.setValue(engine, "Hello World!") | stdexec::then([&] { engine.requestStop(); }));
  engine.run();
  auto res = input.getValue();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, "Hello World!");
}

namespace ht = heph::telemetry;
class MockLogSink final : public ht::ILogSink {
public:
  void send(const ht::LogEntry& s) override {
    if (s.level == heph::WARN && s.message == detail::OutputConnections::INPUT_OVERFLOW_WARNING) {
      ++num_messages;
      fmt::println(stderr, "{}", fmt::join(s.fields, ", "));
    }
  }

  std::atomic<unsigned> num_messages{ 0 };
};

static constexpr auto TIMEOUT = std::chrono::milliseconds(10);

TEST(InputOutput, QueuedInputOutputDelay) {
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  NodeEngine engine{ {} };
  exec::async_scope scope;
  DummyOperation dummy;

  QueuedInput<std::string> input{ &dummy, "input" };
  Output<std::string> output{ &dummy, "output" };
  input.connectTo(output);
  EXPECT_EQ(input.setValue("Hello World!"), InputState::OK);
  std::optional<std::string> res;
  scope.spawn(output.setValue(engine, "Hello World Again!") | stdexec::then([&] { engine.requestStop(); }));
  scope.spawn(engine.scheduler().scheduleAfter(TIMEOUT) | stdexec::let_value([&] { return input.get(); }) |
              stdexec::then([&](std::string value) { res.emplace(std::move(value)); }));
  // scope.spawn(output.setValue(engine, "Hello World!"));
  engine.run();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, "Hello World!");
  res = input.getValue();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, "Hello World Again!");
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages, 1);
}

TEST(InputOutput, QueuedInputOutputDelaySimulated) {
  auto mock_sink = std::make_unique<MockLogSink>();
  MockLogSink* sink_ptr = mock_sink.get();
  heph::telemetry::registerLogSink(std::move(mock_sink));

  NodeEngine engine{
    { .context_config = { .io_ring_config = {},
                          .timer_options = { .clock_mode = concurrency::io_ring::ClockMode::SIMULATED } },
      .number_of_threads = 1,
      .endpoints = {} }
  };
  exec::async_scope scope;
  DummyOperation dummy;

  QueuedInput<std::string> input{ &dummy, "input" };
  Output<std::string> output{ &dummy, "output" };
  input.connectTo(output);
  EXPECT_EQ(input.setValue("Hello World!"), InputState::OK);
  std::optional<std::string> res;
  scope.spawn(output.setValue(engine, "Hello World Again!") | stdexec::then([&] { engine.requestStop(); }));
  scope.spawn(engine.scheduler().scheduleAfter(TIMEOUT) | stdexec::let_value([&] { return input.get(); }) |
              stdexec::then([&](std::string value) { res.emplace(std::move(value)); }));
  // scope.spawn(output.setValue(engine, "Hello World!"));
  engine.run();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, "Hello World!");
  res = input.getValue();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, "Hello World Again!");
  heph::telemetry::flushLogEntries();
  EXPECT_GE(sink_ptr->num_messages.load(), 1);
}

TEST(InputOutput, handleStopped) {
  NodeEngine engine{ {} };
  [[maybe_unused]] auto dummy = engine.createNode<InputOperation>();

  exec::async_scope scope;
  scope.spawn(engine.scheduler().scheduleAfter(TIMEOUT) | stdexec::then([&] { engine.requestStop(); }));

  engine.run();
}

TEST(InputOutput, QueuedInputWhenAny) {
  NodeEngine engine{ {} };
  DummyOperation dummy;
  QueuedInput<std::string, InputPolicy<1, RetrievalMethod::POLL>> input1{ &dummy, "input1" };
  QueuedInput<int> input2{ &dummy, "input2" };
  QueuedInput<double, InputPolicy<1, RetrievalMethod::POLL>> input3{ &dummy, "input3" };
  QueuedInput<std::string> input4{ &dummy, "input4" };

  {
    auto res = stdexec::sync_wait_with_variant(exec::when_any(input1.get(), input2.get()));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->index(), 0);
    EXPECT_FALSE(std::get<0>(std::get<0>(*res)).has_value());
  }
  {
    auto res = stdexec::sync_wait_with_variant(exec::when_any(input1.get(), input3.get()));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->index(), 0);
    EXPECT_FALSE(std::get<0>(std::get<0>(*res)).has_value());
  }
  {
    EXPECT_EQ(input1.setValue("..."), InputState::OK);
    EXPECT_EQ(input3.setValue(9.0), InputState::OK);
    auto res = stdexec::sync_wait_with_variant(exec::when_any(input1.get(), input3.get()));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->index(), 0);
    EXPECT_TRUE(std::get<0>(std::get<0>(*res)).has_value());
    auto res2 = input3.getValue();
    EXPECT_TRUE(res2.has_value());
  }
  {
    EXPECT_EQ(input1.setValue("..."), InputState::OK);
    EXPECT_EQ(input2.setValue(4), InputState::OK);
    auto res = stdexec::sync_wait_with_variant(exec::when_any(input1.get(), input2.get()));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->index(), 0);
    EXPECT_TRUE(std::get<0>(std::get<0>(*res)).has_value());
    auto res2 = input2.getValue();
    EXPECT_TRUE(res2.has_value());
  }
  {
    EXPECT_EQ(input2.setValue(4), InputState::OK);
    EXPECT_EQ(input4.setValue("b"), InputState::OK);
    auto res = stdexec::sync_wait_with_variant(exec::when_any(input2.get(), input4.get()));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->index(), 0);
    EXPECT_EQ(std::get<0>(std::get<0>(*res)), 4);
    auto res2 = input4.getValue();
    EXPECT_EQ(res2, "b");
  }
  {
    exec::async_scope scope;
    Output<std::string> output{ &dummy, "output" };
    input4.connectTo(output);

    std::variant<int, std::string> res;
    scope.spawn(engine.scheduler().scheduleAfter(TIMEOUT) |
                stdexec::let_value([&] { return output.setValue(engine, "..."); }));
    scope.spawn(exec::when_any(input2.get(), input4.get()) |
                stdexec::then([&]<typename ValueT>(ValueT value) {
                  engine.requestStop();
                  res.emplace<ValueT>(std::move(value));
                }));
    engine.run();
    EXPECT_EQ(res.index(), 1);
    EXPECT_EQ(std::get<1>(res), "...");
    auto res2 = input2.getValue();
    EXPECT_FALSE(res2.has_value());
    auto res3 = input4.getValue();
    EXPECT_FALSE(res3.has_value());
  }
}

struct OptionalOutputData {
  std::size_t iteration{ 0 };
  bool propagate = false;
};

struct OptionalOutputOperation : Node<OptionalOutputOperation, OptionalOutputData> {
  static auto trigger(OptionalOutputOperation& self) {
    if (self.data().iteration != 0) {
      self.engine().requestStop();
    }
    return stdexec::just();
  }
  static constexpr int VALUE{ 4711 };
  static auto execute(OptionalOutputOperation& self) -> std::optional<int> {
    ++self.data().iteration;
    if (self.data().propagate) {
      return std::optional{ VALUE };
    }
    return std::nullopt;
  }
};

TEST(InputOutput, QueuedInputOptionalOutput) {
  {
    NodeEngine engine{ {} };
    DummyOperation dummy;
    auto op = engine.createNode<OptionalOutputOperation>();
    QueuedInput<int> input{ &dummy, "input" };
    input.connectTo(op);

    engine.run();
    EXPECT_FALSE(input.getValue().has_value());
  }
  {
    DummyOperation dummy;
    NodeEngine engine{ {} };
    auto op = engine.createNode<OptionalOutputOperation>();
    QueuedInput<int> input{ &dummy, "input" };
    input.connectTo(op);
    op->data().propagate = true;

    engine.run();
    auto res = input.getValue();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, OptionalOutputOperation::VALUE);
  }
}

TEST(InputOutput, AccumulatedInputBase) {
  DummyOperation dummy;
  auto accumulator = [](int value, std::vector<int> state) {
    state.push_back(value);
    return state;
  };
  AccumulatedInputBase<int, std::vector<int>, decltype(accumulator), InputPolicy<2>> input{ &dummy,
                                                                                            accumulator,
                                                                                            "input" };

  auto res = input.getValue();
  EXPECT_FALSE(res.has_value());

  EXPECT_EQ(input.setValue(0), InputState::OK);
  EXPECT_EQ(input.setValue(1), InputState::OK);

  res = input.getValue();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, std::vector({ 0, 1 }));

  EXPECT_EQ(input.setValue(0), InputState::OK);
  EXPECT_EQ(input.setValue(1), InputState::OK);
  EXPECT_EQ(input.setValue(2), InputState::OVERFLOW);
  res = input.getValue();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(*res, std::vector({ 0, 1 }));

  EXPECT_EQ(input.setValue(0), InputState::OK);
  EXPECT_EQ(input.setValue(1), InputState::OK);
  stdexec::sync_wait(input.get() | stdexec::then([](const std::vector<int>& state) {
                       EXPECT_EQ(state, std::vector({ 0, 1 }));
                     }));
}

struct AccumulatedNodeData {};

struct AccumulatedNode : Node<AccumulatedNode, AccumulatedNodeData> {
  using AccumulatedPolicyT = heph::conduit::InputPolicy<3, heph::conduit::RetrievalMethod::POLL,
                                                        heph::conduit::SetMethod::OVERWRITE>;
  heph::conduit::AccumulatedInput<std::vector<int>, AccumulatedPolicyT> input{
    this,
    [](int value, std::vector<int> state) {
      state.push_back(value);
      return state;
    },
    "test_accumulated_input",
  };

  static auto period() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return std::chrono::milliseconds(100);
  }

  static auto trigger(AccumulatedNode& self) {
    return self.input.get();
  }

  static auto execute([[maybe_unused]] AccumulatedNode& self,
                      [[maybe_unused]] const std::optional<std::vector<int>>& value) {
    self.engine().requestStop();
  }
};

TEST(InputOutput, ConstructAccumulatedNode) {
  NodeEngine engine{ {} };
  [[maybe_unused]] auto dummy = engine.createNode<AccumulatedNode>();
  engine.run();
  SUCCEED();
}

static constexpr std::size_t NUM_REPEATS = 1000;
struct NodeCompletionData {
  std::size_t iteration{ 0 };
  std::optional<std::thread::id> producer_id;
};

struct NodeCompletionOperation : Node<NodeCompletionOperation, NodeCompletionData> {
  QueuedInput<int, InputPolicy<1>> input{ this, "input" };

  static auto trigger(NodeCompletionOperation& self) {
    return self.input.get();
  }

  static auto execute(NodeCompletionOperation& self, int value) {
    EXPECT_EQ(value, self.data().iteration);
    EXPECT_NE(self.data().producer_id.value(), std::this_thread::get_id());

    ++self.data().iteration;
    if (self.data().iteration == NUM_REPEATS) {
      self.engine().requestStop();
    }
  }
};

TEST(InputOutput, ConcurrrentAccess) {
  NodeEngine engine{ {} };
  auto dummy = engine.createNode<NodeCompletionOperation>();

  std::mutex mtx;
  std::condition_variable cv;
  std::thread producer{ [&] {
    {
      const std::scoped_lock l{ mtx };
      dummy->data().producer_id.emplace(std::this_thread::get_id());
      cv.notify_all();
    }
    for (std::size_t i = 0; i != NUM_REPEATS; ++i) {
      while (dummy->input.setValue(i) != InputState::OK) {
      }
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  } };
  {
    std::unique_lock l{ mtx };
    cv.wait(l, [&] { return dummy->data().producer_id.has_value(); });
  }
  engine.run();

  producer.join();
}
// NOLINTEND(bugprone-unchecked-optional-access)

}  // namespace heph::conduit::tests
