//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>

#include <exec/when_any.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
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

struct NodeMerger : Node<NodeMerger> {
  explicit NodeMerger(heph::conduit::NodeEngine& engine)
    : node1_(engine.createNode<Node1>()), node2_(engine.createNode<Node2>()) {
    input_from_output_.connectTo(node2_);
    node1_->input.connectTo(output_from_input_);
    input_from_output_.connectTo(node2_);
  }

  static auto name() -> std::string_view {
    return "NodeMerger";
  }

  using InputPolicyT = InputPolicy<1, RetrievalMethod::BLOCK, SetMethod::OVERWRITE>;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes
  QueuedInput<double, InputPolicyT> input{ this, "input" };
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes
  Output<float> output{ this, "output" };

  static auto trigger(NodeMerger& self) {
    return exec::when_any(self.input.get(), self.input_from_output_.get());
  }

  static auto execute(NodeMerger& self, double value) {
    return self.output_from_input_.setValue(self.engine(), value);
  }

  static auto execute(NodeMerger& self, float value) {
    return self.output.setValue(self.engine(), value);
  }

private:
  NodeHandle<Node1> node1_;
  NodeHandle<Node2> node2_;

  QueuedInput<float, InputPolicyT> input_from_output_{ this, "input_from_output" };
  Output<double> output_from_input_{ this, "output1" };
};

struct ProducerNode : Node<ProducerNode> {
  static constexpr auto NAME = "producer";
  static constexpr auto PERIOD = std::chrono::milliseconds(1000);

  static auto execute() {
    static double counter = 0;
    return counter++;
  }
};

struct ConsumerNode : Node<ProducerNode> {
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
    auto super_node = engine.createNode<heph::conduit::NodeMerger>();
    auto consumer = engine.createNode<heph::conduit::ConsumerNode>();

    super_node->input.connectTo(producer);
    consumer->input.connectTo(super_node->output);

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine]() { engine.requestStop(); });
    engine.run();

  } catch (...) {
    fmt::println("unexcepted exception...");
  }
}
