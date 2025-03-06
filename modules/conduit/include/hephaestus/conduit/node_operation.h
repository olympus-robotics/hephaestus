
//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <compare>
#include <ranges>
#include <set>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <exec/repeat_effect_until.hpp>
#include <fmt/ranges.h>
#include <hephaestus/conduit/node_operation_handle.h>

#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"

namespace heph::conduit {

struct DataflowGraph {
  std::string toDot() {
    return fmt::format("digraph G {{\n{}\n}}", fmt::join(edges | std::views::transform([](auto& edge) {
                                                           return fmt::format("{} -> {}", edge.source.name(),
                                                                              edge.destination.name());
                                                         }),
                                                         "\n"));
  }
  struct Edge {
    NodeOperationHandle source;
    NodeOperationHandle destination;

    friend auto operator<=>(Edge const& lhs, Edge const& rhs) {
      return std::forward_as_tuple(lhs.source.name(), lhs.destination.name()) <=>
             std::forward_as_tuple(rhs.source.name(), rhs.destination.name());
    }
  };

  void addEdge(NodeOperationHandle source, NodeOperationHandle destination) {
    edges.insert({ source, destination });
  }

  std::set<Edge> edges;
};

template <typename OutputType>
struct OutputConnections {
  struct OutputConnection {
    using continuation_type = InputState (*)(void* input, OutputType const&);
    void* input;
    continuation_type set_value;
    // TODO: figure out how to move this out of the state
    std::size_t retry;
    bool triggered;
  };

  template <typename Input>
  void addConnection(Input& input) {
    continuations.push_back(OutputConnection{ &input,
                                              [](void* input_ptr, OutputType const& data) {
                                                return static_cast<Input*>(input_ptr)->setValue(data);
                                              },
                                              0, false });
  }

  auto propagate(Context& context) {
    return stdexec::let_value([this, &context](auto const& output) {
      auto apply = stdexec::then([this, output]() {
        // TODO: If the output is an optional, we skip the forwarding as it marks the
        // operation as disabled if it has no value.
        bool done = true;
        for (auto& continuation : continuations) {
          if (continuation.triggered) {
            continue;
          }
          auto res = continuation.set_value(continuation.input, output);
          if (res == InputState::OVERFLOW) {
            done = false;
            ++continuation.retry;
            // heph::log(heph::WARN, "conduit", "queue overflow", "name TBD", "retry", continuation.retry);
            continue;
          }
          continuation.triggered = true;
        }
        ++retry;
        return done;
      });
      // TODO: find better way to timeout based on the inputs timing...
      static constexpr std::array TIMEOUTS = {
        std::chrono::milliseconds(0),    std::chrono::milliseconds(100), std::chrono::milliseconds(200),
        std::chrono::milliseconds(400),  std::chrono::milliseconds(800), std::chrono::milliseconds(500),
        std::chrono::milliseconds(1600), std::chrono::milliseconds(3200)
      };
      return exec::repeat_effect_until(
                 stdexec::just() | stdexec::then([this] { return TIMEOUTS[this->retry % TIMEOUTS.size()]; }) |
                 stdexec::let_value([&context](std::chrono::steady_clock::duration duration) {
                   return context.scheduleAfter(duration);
                 }) |
                 std::move(apply)) |
             stdexec::then([this] {
               for (auto& continuation : continuations) {
                 continuation.triggered = false;
                 continuation.retry = 0;
               }
               retry = 0;
             });
    });
  }

  std::size_t retry{ 0 };
  std::vector<OutputConnection> continuations;
};

template <>
struct OutputConnections<void> {
  auto propagate(Context& /*context*/) {
    return stdexec::then([] {});
  }
};

template <typename T>
struct Output {
  template <typename Node>
  Output(Node* node, std::string_view name) : node(node), name(name) {
  }

  auto setValue(Context& context, T const& value) {
    return stdexec::just(value) | connections.propagate(context);
  }

  template <typename Input>
  void connectTo(Input& input) {
    static_assert(std::is_same_v<T, typename Input::type>, "Types must match");
    input.setParent(node);
    node.addChild(input.node);
    connections.addConnection(input);
  }

  NodeOperationHandle node;
  std::string_view name;
  OutputConnections<T> connections;
};

struct ErasedOperation {
  ErasedOperation() = default;
  ~ErasedOperation() = default;
  ErasedOperation(const ErasedOperation&) = delete;
  ErasedOperation(ErasedOperation&&) = delete;
  auto operator=(const ErasedOperation&) -> ErasedOperation& = delete;
  auto operator=(ErasedOperation&&) -> ErasedOperation& = delete;

  using deleter_type = void (*)(void*);
  using ptr_type = std::unique_ptr<void, deleter_type>;
  ptr_type data{ nullptr, nullptr };

  void reset() {
    data.reset();
  }

  template <typename Operation>
  void emplace(Operation operation) {
    using OperationType = decltype(operation());

    data = ptr_type(new OperationType(operation()), [](void* ptr) {
      // This is the deleter of the unique pointer. As such, it is
      // ownining by definition and we need to silence the linter
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      delete static_cast<OperationType*>(ptr);
    });
    static_cast<OperationType*>(data.get())->start();
  }
};

struct Receiver {
  using is_receiver = void;

  template <typename... Ts>
  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  void set_value(Ts&&... /*ts*/) noexcept {
  }

  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  void set_stopped() noexcept {
  }

  template <typename Error>
  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  void set_error(Error&& /*error*/) noexcept {
    // TOOO: error handling...
  }
};

template <typename Operation, typename OutputType>
// NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
struct NodeOperation {
  auto run(Context& context) -> bool {
    DataflowGraph g;
    return run(g, context);
  }
  auto run(DataflowGraph& g, Context& context) -> bool {
    // Check if already started, in this case, we return false to signal we already got visited.
    if (runner_operation.data) {
      return false;
    }
    // First, traverse to all children
    // This is needed since the children depend on our output.
    // If we would first start ourselves or the parents, we might
    // end up spamming their input queue before they can react
    for (auto& child : children()) {
      g.addEdge(NodeOperationHandle{ this }, child);
      child.run(g, context);
    }
    // Then start ourselves
    if (!runner_operation.data) {
      fmt::println("  Running {}", operation().name);
      // TODO: error handling
      auto runner = exec::repeat_effect_until(context.schedule() |
                                              stdexec::let_value([&] { return execute(context); }) |
                                              stdexec::then([&context] { return !context.isRunning(); }));
      runner_operation.emplace([&] { return stdexec::connect(std::move(runner), Receiver{}); });
    }

    // And traverse the parents
    for (auto& parent : parents()) {
      g.addEdge(parent, NodeOperationHandle{ this });
      parent.run(g, context);
    }

    return true;
  }

  auto execute(Context& context) {
    // TODO: check that operator is overloaded properly
    auto trigger = operation().trigger(context);
    using trigger_type = decltype(trigger);
    using trigger_values_variant = stdexec::value_types_of_t<trigger_type>;
    using trigger_values = std::variant_alternative_t<0, trigger_values_variant>;
    auto invoke_operation = [this, &context]<typename... Ts>(Ts&&... ts) {
      if constexpr (std::is_invocable_v<Operation&, Context&, Ts...>) {
        return operation()(context, std::forward<Ts>(ts)...);
      } else {
        (void)context;
        return operation()(std::forward<Ts>(ts)...);
      }
    };
    using result_type = decltype(std::apply(invoke_operation, std::declval<trigger_values>()));
    return std::move(trigger) | [&] {
      if constexpr (stdexec::sender<result_type>) {
        return stdexec::let_value(invoke_operation);
      } else {
        return stdexec::then(invoke_operation);
      }
    }() | output.propagate(context);
  }

  template <typename Input>
  void connectTo(Input& input)
    requires(!std::is_same_v<OutputType, void>)
  {
    static_assert(std::is_same_v<OutputType, typename Input::type>, "Types must match");
    input.setParent(this);
    output.addConnection(input);
  }

  auto parents() -> std::vector<NodeOperationHandle> {
    std::vector<NodeOperationHandle> res;
    res.reserve(inputs.size());
    for (auto& input : inputs) {
      auto parent = input.parent(input.ptr);
      if (parent.node != nullptr) {
        res.push_back(parent);
      }
    }
    return res;
  }

  auto getName() {
    return operation().name;
  }

  auto operation() -> Operation& {
    return *static_cast<Operation*>(this);
  }
  struct InputHandle {
    void* ptr;
    NodeOperationHandle (*parent)(void*);
  };

  template <typename Input>
  void registerInput(Input* input) {
    inputs.emplace_back(input, [](void* ptr) { return static_cast<Input*>(ptr)->parent; });
  }

  void addChild(NodeOperationHandle child) {
    // TODO: make children unique...
    child_nodes.push_back(child);
  }
  auto children() -> std::vector<NodeOperationHandle> {
    return child_nodes;
  }

  ErasedOperation runner_operation;
  std::vector<InputHandle> inputs;
  OutputConnections<OutputType> output;
  std::vector<NodeOperationHandle> child_nodes;
};
}  // namespace heph::conduit
