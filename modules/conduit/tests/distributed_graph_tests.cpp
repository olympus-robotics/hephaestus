//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <fmt/base.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/executor.h"
#include "hephaestus/conduit/forwarding_input.h"
#include "hephaestus/conduit/forwarding_output.h"
#include "hephaestus/conduit/graph.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/stepper.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/types_proto/numeric_value.h"  // IWYU pragma: keep

namespace heph::conduit {

struct Node0 : NodeDescriptionDefaults<Node0> {
  static constexpr std::string_view NAME = "node0";
  struct Inputs {
    Input<int> input{ "input" };
    Input<int> remote_input{ "remote_input" };
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
    auto remote_res = inputs.remote_input.value() + 1;
    EXPECT_EQ(res % 4, 1);
    EXPECT_EQ(res, remote_res);
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

    // Connect remote
    node2->outputs.output.connectToPartner(node0->inputs.remote_input);
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

  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
    ++executed;
  }
};

static constexpr std::size_t NUMBER_OF_REPEATS = 100;

TEST(Graph, Distributed) {
  const GraphConfig config0{
    .prefix = "test_0",
    .partners = { "test_1" },
  };
  Graph<RootStepper> g0{ config0 };

  const GraphConfig config1{
    .prefix = "test_1",
    .partners = { "test_0" },
  };
  Graph<RootStepper> g1{ config1 };

  const ExecutorConfig config{
    .runners = { {
      .selector = {".*root.node2.*" },
      .context_config = {},
    },
      {
        .selector = ".*",
        .context_config = {},
      },
    },
    .acceptor = {
      .endpoints = {heph::net::Endpoint::createIpV4("127.0.0.1")},
      .partners = {},
    },
  };
  Executor executor0{ config };
  Executor executor1{ config };

  executor0.addPartner("test_1", executor1.endpoints()[0]);
  executor1.addPartner("test_0", executor0.endpoints()[0]);

  Input<int> test0{ "test0" };
  g0.root()->children.node0->outputs.output.connect(test0);
  Input<int> test1{ "test1" };
  g1.root()->children.node0->outputs.output.connect(test1);

  stdexec::sync_wait(g0.root()->children.node0->inputs.input.setValue(0));
  stdexec::sync_wait(g0.root()->children.node0->inputs.remote_input.setValue(0));
  stdexec::sync_wait(g1.root()->children.node0->inputs.input.setValue(0));
  stdexec::sync_wait(g1.root()->children.node0->inputs.remote_input.setValue(0));

  executor0.spawn(g0);
  executor1.spawn(g1);

  for (std::size_t i = 0; i != NUMBER_OF_REPEATS; ++i) {
    stdexec::sync_wait(stdexec::when_all(test0.trigger(SchedulerT{}), test1.trigger(SchedulerT{})));
    EXPECT_TRUE(test0.hasValue());
    EXPECT_TRUE(test1.hasValue());
    const int res0 = test0.value();
    EXPECT_EQ(res0 % 4, 1);
    const int res1 = test1.value();
    EXPECT_EQ(res1 % 4, 1);
  }
  fmt::println(stderr, "done");
  executor0.requestStop();
  executor0.join();
  executor1.requestStop();
  executor1.join();

  auto expected_executed =
      ::testing::AllOf(::testing::Ge(NUMBER_OF_REPEATS - 1), ::testing::Le(NUMBER_OF_REPEATS + 2));
  EXPECT_EQ(g0.stepper().executed, 0);
  EXPECT_THAT(g0.stepper().node0.executed, expected_executed);
  EXPECT_EQ(g0.stepper().node1.executed, 0);
  EXPECT_THAT(g0.stepper().node1.node10.executed, expected_executed);
  EXPECT_THAT(g0.stepper().node1.node11.executed, expected_executed);
  EXPECT_THAT(g0.stepper().node2.executed, expected_executed);

  EXPECT_EQ(g0.stepper().node0.thread_id, g0.stepper().node1.node10.thread_id);
  EXPECT_EQ(g0.stepper().node0.thread_id, g0.stepper().node1.node11.thread_id);
  EXPECT_NE(g0.stepper().node0.thread_id, g0.stepper().node2.thread_id);

  EXPECT_EQ(g1.stepper().executed, 0);
  EXPECT_THAT(g1.stepper().node0.executed, expected_executed);
  EXPECT_EQ(g1.stepper().node1.executed, 0);
  EXPECT_THAT(g1.stepper().node1.node10.executed, expected_executed);
  EXPECT_THAT(g1.stepper().node1.node11.executed, expected_executed);
  EXPECT_THAT(g1.stepper().node2.executed, expected_executed);

  EXPECT_EQ(g1.stepper().node0.thread_id, g1.stepper().node1.node10.thread_id);
  EXPECT_EQ(g1.stepper().node0.thread_id, g1.stepper().node1.node11.thread_id);
  EXPECT_NE(g1.stepper().node0.thread_id, g1.stepper().node2.thread_id);
}

}  // namespace heph::conduit
