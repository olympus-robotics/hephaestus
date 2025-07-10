//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#include <fmt/base.h>

#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/node_handle.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types_proto/numeric_value.h"
#include "hephaestus/utils/signal_handler.h"

namespace heph::conduit {
struct Node1 : Node<Node1> {
  static auto name() -> std::string_view {
    return "Node1";
  }
  using InputPolicyT = InputPolicy<1, RetrievalMethod::BLOCK, SetMethod::OVERWRITE>;

  QueuedInput<double, InputPolicyT> input{ this, "input" };

  static auto trigger(Node1& self) {
    return self.input.get();
  }

  static auto execute(double value) {
    return value;
  }
};

struct Node2 : Node<Node2> {
  static auto name() -> std::string_view {
    return "Node2";
  }
  using InputPolicyT = InputPolicy<1, RetrievalMethod::BLOCK, SetMethod::OVERWRITE>;

  QueuedInput<double, InputPolicyT> input{ this, "input" };

  static auto trigger(Node2& self) {
    return self.input.get();
  }

  static auto execute(double value) {
    return static_cast<float>(value);
  }
};

class Graph {
public:
  explicit Graph(NodeEngine& engine)
    : node1_(engine.createNode<Node1>()), node2_(engine.createNode<Node2>()) {
    node2_->input.connectTo(node1_);
  }

  [[nodiscard]] auto input() {
    return &node1_->input;
  }

  [[nodiscard]] auto output() -> NodeHandle<Node2>& {
    return node2_;
  }

private:
  NodeHandle<Node1> node1_;
  NodeHandle<Node2> node2_;
};

struct ProducerNode : Node<ProducerNode> {
  static constexpr auto NAME = "producer";
  static constexpr auto PERIOD = std::chrono::milliseconds(1000);

  static auto execute() {
    static double counter = 0;
    return counter++;
  }
};

struct ConsumerNode : Node<ConsumerNode> {
  static constexpr auto NAME = "Consumer";

  using InputPolicyT = InputPolicy<1, RetrievalMethod::BLOCK, SetMethod::OVERWRITE>;
  QueuedInput<float, InputPolicyT> input{ this, "input" };

  static auto trigger(ConsumerNode& self) {
    return self.input.get();
  }

  static auto execute(float value) {
    fmt::println("value: {}", value);
  }
};

}  // namespace heph::conduit

auto main() -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
    heph::conduit::NodeEngine engine{ {} };

    auto producer = engine.createNode<heph::conduit::ProducerNode>();
    auto graph = heph::conduit::Graph(engine);
    auto consumer = engine.createNode<heph::conduit::ConsumerNode>();

    graph.input()->connectTo(producer);
    consumer->input.connectTo(graph.output());

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine]() { engine.requestStop(); });
    engine.run();

  } catch (...) {
    fmt::println("unexcepted exception...");
  }

  return EXIT_SUCCESS;
}
