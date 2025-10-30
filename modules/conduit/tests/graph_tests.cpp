//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <thread>

#include <exec/static_thread_pool.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>
#include <fmt/format.h>
#include <fmt/std.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/executor.h"
#include "hephaestus/conduit/forwarding_input.h"
#include "hephaestus/conduit/forwarding_output.h"
#include "hephaestus/conduit/graph.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/stepper.h"

namespace heph::conduit {

struct JustInput : BasicInput {
  JustInput() : BasicInput("just") {
  }
  auto doTrigger(SchedulerT /*scheduler*/) -> SenderT final {
    return stdexec::just(true);
  }
  void handleCompleted() final {
  }
};

struct Dummy : NodeDescriptionDefaults<Dummy> {
  static constexpr auto NAME = "receiver";
  struct Inputs {
    JustInput just;
  };
};

struct ReceiverStep : StepperDefaults<Dummy> {
  bool executed = false;
  Executor* executor;
  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
    executed = true;
    executor->requestStop();
  }
};

TEST(Graph, SingleStep) {
  GraphConfig config{
    .prefix = "test",
    .partners = {},
  };
  Executor executor;
  Graph<ReceiverStep> g{ config, {} };
  g.stepper().executor = &executor;

  executor.spawn(g);
  executor.join();

  EXPECT_TRUE(g.stepper().executed);
  EXPECT_EQ(g.root()->name(), "/test/receiver");
}
static constexpr std::size_t NUMBER_OF_REPEATS = 100;

struct RepeaterStep : StepperDefaults<Dummy> {
  std::size_t executed{ 0 };
  Executor* executor;
  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
    if (executed == NUMBER_OF_REPEATS) {
      executor->requestStop();
      return;
    }
    ++executed;
  }
};

TEST(Graph, RepeatedStep) {
  GraphConfig config{
    .prefix = "test",
    .partners = {},
  };
  Executor executor;
  Graph<RepeaterStep> g{ config, {} };
  g.stepper().executor = &executor;

  executor.spawn(g);
  executor.join();

  EXPECT_EQ(g.stepper().executed, NUMBER_OF_REPEATS);
  EXPECT_EQ(g.root()->name(), "/test/receiver");
}

struct RepeaterPoolStep : StepperDefaults<Dummy> {
  std::size_t executed{ 0 };
  exec::static_thread_pool* pool;
  std::thread::id thread_id;
  Executor* executor;
  auto step(InputsT& /*inputs*/, OutputsT& /*outputs*/) -> exec::task<void> {
    if (executed == NUMBER_OF_REPEATS) {
      executor->requestStop();
      pool->request_stop();
      co_return;
    }
    co_await stdexec::schedule(pool->get_scheduler());
    EXPECT_NE(thread_id, std::this_thread::get_id());
    ++executed;
  }
};

TEST(Graph, RepeatedPoolStep) {
  GraphConfig config{
    .prefix = "test",
    .partners = {},
  };
  Executor executor;
  exec::static_thread_pool pool{ 2 };
  Graph<RepeaterPoolStep> g{ config, { {}, 0, &pool, std::this_thread::get_id(), &executor } };

  executor.spawn(g);
  executor.join();

  EXPECT_EQ(g.stepper().executed, NUMBER_OF_REPEATS);
  EXPECT_EQ(g.root()->name(), "/test/receiver");
}

struct RepeaterExceptionStep : StepperDefaults<Dummy> {
  std::size_t executed{ 0 };
  Executor* executor;
  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
    if (executed == NUMBER_OF_REPEATS) {
      executor->requestStop();
      return;
    }
    if (executed == NUMBER_OF_REPEATS / 2) {
      heph::panic("muuh");
    }
    ++executed;
  }
};

TEST(Graph, RepeatedExceptionStep) {
  GraphConfig config{
    .prefix = "test",
    .partners = {},
  };
  Executor executor;
  Graph<RepeaterExceptionStep> g{ config, {} };
  g.stepper().executor = &executor;

  executor.spawn(g);
  EXPECT_THROW(executor.join(), heph::Panic);

  EXPECT_EQ(g.stepper().executed, NUMBER_OF_REPEATS / 2);
  EXPECT_EQ(g.root()->name(), "/test/receiver");
}

struct Node0 : NodeDescriptionDefaults<Node0> {
  static constexpr std::string_view NAME = "node0";
  struct Inputs {
    Input<int> input{ "input" };
  };
  struct Outputs {
    Output<int> output{ "output" };
  };
};

struct NodeStepper0 : StepperDefaults<Node0> {
  std::size_t executed{ 0 };
  std::optional<std::thread::id> thread_id;

  void step(InputsT& inputs, OutputsT& outputs) {
    if (thread_id.has_value()) {
      EXPECT_EQ(*thread_id, std::this_thread::get_id());
    } else {
      thread_id.emplace(std::this_thread::get_id());
    }
    auto res = inputs.input.value() + 1;
    EXPECT_EQ(res % 4, 1);
    ++executed;
    outputs.output(res);
  }
};

struct Node10 : NodeDescriptionDefaults<Node10> {
  static constexpr std::string_view NAME = "node10";
  struct Inputs {
    Input<int> input{ "input" };
  };
  struct Outputs {
    Output<int> output{ "output" };
  };
};

struct NodeStepper10 : StepperDefaults<Node10> {
  std::size_t executed{ 0 };
  std::optional<std::thread::id> thread_id;

  void step(InputsT& inputs, OutputsT& outputs) {
    if (thread_id.has_value()) {
      EXPECT_EQ(*thread_id, std::this_thread::get_id());
    } else {
      thread_id.emplace(std::this_thread::get_id());
    }
    auto res = inputs.input.value() + 1;
    EXPECT_EQ(res % 4, 2);
    ++executed;
    outputs.output(res);
  }
};

struct Node11 : NodeDescriptionDefaults<Node11> {
  static constexpr std::string_view NAME = "node11";
  struct Inputs {
    Input<int> input{ "input" };
  };
  struct Outputs {
    Output<int> output{ "output" };
  };
};

struct NodeStepper11 : StepperDefaults<Node11> {
  std::size_t executed{ 0 };
  std::optional<std::thread::id> thread_id;

  void step(InputsT& inputs, OutputsT& outputs) {
    if (thread_id.has_value()) {
      EXPECT_EQ(*thread_id, std::this_thread::get_id());
    } else {
      thread_id.emplace(std::this_thread::get_id());
    }
    auto res = inputs.input.value() + 1;
    EXPECT_EQ(res % 4, 3);
    ++executed;
    outputs.output(res);
  }
};

struct Node1 : NodeDescriptionDefaults<Node1> {
  static constexpr std::string_view NAME = "node1";
  struct Inputs {
    ForwardingInput<int> input{ "input" };
  };
  struct Outputs {
    ForwardingOutput<int> output{ "output" };
  };
  struct Children {
    Node<Node10> node10;
    Node<Node11> node11;
  };
  struct ChildrenConfig {
    Node<Node10>::StepperT node10;
    Node<Node11>::StepperT node11;
  };
  static void connect(Inputs& inputs, Outputs& outputs, Children& children) {
    auto& [node10, node11] = children;
    inputs.input.forward(node10->inputs.input);
    node10->outputs.output.connect(node11->inputs.input);

    outputs.output.forward(node11->outputs.output);
  }
};

struct NodeStepper1 : StepperDefaults<Node1> {
  std::size_t executed{ 0 };

  NodeStepper10 node10;
  NodeStepper11 node11;

  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    return {
      .node10 = node10,
      .node11 = node11,
    };
  }

  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
    ++executed;
  }
};

struct Node2 : NodeDescriptionDefaults<Node2> {
  static constexpr std::string_view NAME = "node2";
  struct Inputs {
    Input<int> input{ "input" };
  };
  struct Outputs {
    Output<int> output{ "output" };
  };
};

struct NodeStepper2 : StepperDefaults<Node2> {
  std::size_t executed{ 0 };
  std::optional<std::thread::id> thread_id;

  void step(InputsT& inputs, OutputsT& outputs) {
    if (thread_id.has_value()) {
      EXPECT_EQ(*thread_id, std::this_thread::get_id());
    } else {
      thread_id.emplace(std::this_thread::get_id());
    }
    auto res = inputs.input.value() + 1;
    EXPECT_EQ(res % 4, 0);
    ++executed;
    outputs.output(res);
  }
};

struct Root : NodeDescriptionDefaults<Root> {
  static constexpr std::string_view NAME = "root";
  struct Children {
    Node<Node0> node0;
    Node<Node1> node1;
    Node<Node2> node2;
  };
  struct ChildrenConfig {
    Node<Node0>::StepperT node0;
    Node<Node1>::StepperT node1;
    Node<Node2>::StepperT node2;
  };

  static void connect(Inputs& /*inputs*/, Outputs& /*outputs*/, Children& children) {
    auto& [node0, node1, node2] = children;
    node0->outputs.output.connect(node1->inputs.input);
    node1->outputs.output.connect(node2->inputs.input);
    node2->outputs.output.connect(node0->inputs.input);
  }
};

struct RootStepper : StepperDefaults<Root> {
  std::size_t executed{ 0 };

  NodeStepper0 node0;
  NodeStepper1 node1;
  NodeStepper2 node2;

  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    return {
      .node0 = node0,
      .node1 = node1,
      .node2 = node2,
    };
  }
  void connect(InputsT& /*inputs*/, OutputsT& /*outputs*/, ChildrenT& children) {
    stdexec::sync_wait(children.node0->inputs.input.setValue(0));
  }

  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
    ++executed;
  }
};

TEST(Graph, Connections) {
  GraphConfig config{
    .prefix = "test",
    .partners = {},
  };
  Graph<RootStepper> g{ config, {} };

  Executor executor{ { .runners = { {
                           .selector = {".*root.node2.*" },
                           .context_config = {},
                       },
      {
          .selector = ".*",
          .context_config = {},
      },
  },
                       .acceptor = {} } };

  Input<int> test{ "test" };
  g.root()->children.node0->outputs.output.connect(test);
  executor.spawn(g);

  for (std::size_t i = 0; i != NUMBER_OF_REPEATS; ++i) {
    stdexec::sync_wait(test.trigger(SchedulerT{}));
    EXPECT_TRUE(test.hasValue());
    int res = test.value();
    EXPECT_EQ(res % 4, 1);
  }
  executor.requestStop();
  executor.join();

  EXPECT_EQ(g.stepper().executed, 0);
  auto expected_executed =
      ::testing::AllOf(::testing::Ge(NUMBER_OF_REPEATS - 1), ::testing::Le(NUMBER_OF_REPEATS + 2));
  EXPECT_THAT(g.stepper().node0.executed, expected_executed);
  EXPECT_EQ(g.stepper().node1.executed, 0);
  EXPECT_THAT(g.stepper().node1.node10.executed, expected_executed);
  EXPECT_THAT(g.stepper().node1.node11.executed, expected_executed);
  EXPECT_THAT(g.stepper().node2.executed, expected_executed);

  EXPECT_EQ(g.stepper().node0.thread_id, g.stepper().node1.node10.thread_id);
  EXPECT_EQ(g.stepper().node0.thread_id, g.stepper().node1.node11.thread_id);
  EXPECT_NE(g.stepper().node0.thread_id, g.stepper().node2.thread_id);
}

}  // namespace heph::conduit
