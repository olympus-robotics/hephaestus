//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
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

// NOLINTBEGIN(bugprone-unchecked-optional-access)

namespace heph::conduit::tests {
struct DummyOperation : Node<DummyOperation> {
  static auto trigger() {
    return stdexec::just();
  }
  auto operator()() {
  }
};

TEST(InputOutput, QueuedInputPolling) {
  DummyOperation dummy;
  QueuedInput<int, InputPolicy<1, RetrievalMethod::POLL>> input{ &dummy };

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
  QueuedInput<int, InputPolicy<1, RetrievalMethod::BLOCK>> input{ &dummy };

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
  auto operator()() -> int {
    return VALUE;
  }
};

struct InputOperation : Node<InputOperation> {
  QueuedInput<int> input1{ this };
  bool called{ false };

  auto trigger() {
    return input1.get();
  }

  void operator()(NodeEngine& engine, int input) {
    EXPECT_EQ(input, OutputOperation::VALUE);
    called = true;
    engine.requestStop();
  }
};

TEST(InputOutput, QueuedInputBasicInputOutput) {
  NodeEngine engine{ {} };

  OutputOperation out;
  engine.addNode(out);
  InputOperation in;
  engine.addNode(in);

  in.input1.connectTo(out);
  // connect(out, in.input1);
  engine.run();
  EXPECT_TRUE(in.called);
}

TEST(InputOutput, QueuedInputExplicitOutput) {
  NodeEngine engine{ {} };
  exec::async_scope scope;
  DummyOperation dummy;

  QueuedInput<std::string> input{ &dummy };
  Output<std::string> output{ &dummy };
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

  QueuedInput<std::string> input{ &dummy };
  Output<std::string> output{ &dummy };
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
      .number_of_threads = 1 }
  };
  exec::async_scope scope;
  DummyOperation dummy;

  QueuedInput<std::string> input{ &dummy };
  Output<std::string> output{ &dummy };
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

TEST(InputOutput, QueuedInputWhenAny) {
  DummyOperation dummy;
  QueuedInput<std::string, InputPolicy<1, RetrievalMethod::POLL>> input1{ &dummy };
  QueuedInput<int> input2{ &dummy };
  QueuedInput<double, InputPolicy<1, RetrievalMethod::POLL>> input3{ &dummy };
  QueuedInput<std::string> input4{ &dummy };

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
    NodeEngine engine{ {} };
    exec::async_scope scope;
    Output<std::string> output{ &dummy };
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

struct OptionalOutputOperation : Node<OptionalOutputOperation> {
  std::size_t iteration{ 0 };
  bool propagate = false;
  auto trigger(NodeEngine& engine) const {
    if (iteration != 0) {
      engine.requestStop();
    }
    return stdexec::just();
  }
  static constexpr int VALUE{ 4711 };
  auto operator()() -> std::optional<int> {
    ++iteration;
    if (propagate) {
      return std::optional{ VALUE };
    }
    return std::nullopt;
  }
};

TEST(InputOutput, QueuedInputOptionalOutput) {
  {
    DummyOperation dummy;
    OptionalOutputOperation op;
    NodeEngine engine{ {} };
    QueuedInput<int> input{ &dummy };
    input.connectTo(op);

    engine.addNode(op);
    engine.run();
    EXPECT_FALSE(input.getValue().has_value());
  }
  {
    DummyOperation dummy;
    OptionalOutputOperation op;
    NodeEngine engine{ {} };
    QueuedInput<int> input{ &dummy };
    input.connectTo(op);
    op.propagate = true;

    engine.addNode(op);
    engine.run();
    auto res = input.getValue();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, OptionalOutputOperation::VALUE);
  }
}

TEST(InputOutput, AccumulatedInput) {
  DummyOperation dummy;
  auto accumulator = [](int value, std::vector<int> state) {
    state.push_back(value);
    return state;
  };
  AccumulatedInput<int, std::vector<int>, decltype(accumulator)> input{ &dummy, accumulator };

  auto res = input.getValue();
  EXPECT_FALSE(res.has_value());

  EXPECT_EQ(input.setValue(0), InputState::OK);

  res = input.getValue();
  EXPECT_FALSE(res.has_value());

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
}

// NOLINTEND(bugprone-unchecked-optional-access)

}  // namespace heph::conduit::tests
