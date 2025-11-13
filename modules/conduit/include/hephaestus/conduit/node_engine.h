//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/conduit/detail/input_base.h"
#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node_handle.h"
#include "hephaestus/conduit/remote_node_handler.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/telemetry/log/log.h"

namespace heph::conduit {
struct NodeEngineConfig {
  heph::concurrency::ContextConfig context_config;
  std::string prefix;
  std::uint32_t number_of_threads{ 1 };
  std::vector<heph::net::Endpoint> endpoints;
};

class NodeEngine {
public:
  explicit NodeEngine(const NodeEngineConfig& config);

  void run();
  void requestStop();

  auto getStopToken() {
    return context_.getStopToken();
  }

  auto scheduler() {
    return context_.scheduler();
  }

  auto isCurrent() -> bool {
    return context_.isCurrent();
  }

  auto poolScheduler() {
    return pool_.get_scheduler();
  }

  auto elapsed() {
    return context_.elapsed();
  }

  auto prefix() const -> std::string {
    return prefix_;
  }

  auto endpoints() const -> std::vector<heph::net::Endpoint>;

  template <typename OperatorT, typename... Ts>
  auto createNode(Ts&&... ts) -> NodeHandle<OperatorT>;

  template <typename Output>
  void registerOutput(Output& output);
  template <typename Node>
  void registerImplicitOutput(Node& node);
  template <typename InputT>
  void registerInput(InputT* input) {
    remote_node_handler_.registerInput(*this, input);
  }

  void addConnectionSpecification();
  [[nodiscard]] auto getDotGraph() const -> std::string;

private:
  auto uponError();
  template <typename Node>
  auto uponStopped(Node* node);

  template <typename Node>
  auto createNodeRunner(Node& node);

  struct ConnectionSpecification {
    detail::InputSpecification input;
    detail::OutputSpecification output;
  };

  void addConnectionSpec(ConnectionSpecification spec) {
    connection_specs_.push_back(std::move(spec));
  }

private:  // Optimal fields order: pool_, exception_, scope_, context_
  friend detail::OutputConnections;

  exec::static_thread_pool pool_;
  std::exception_ptr exception_;
  exec::async_scope scope_;
  heph::concurrency::Context context_{ {} };
  std::string prefix_;
  std::vector<heph::net::Endpoint> endpoints_;

  std::vector<std::unique_ptr<detail::NodeBase>> nodes_;

  RemoteNodeHandler remote_node_handler_;

  std::vector<ConnectionSpecification> connection_specs_;
};

namespace detail {
template <typename InputT>
void registerInput(NodeEngine& engine, InputT* input) {
  engine.registerInput(input);
}
}  // namespace detail

template <typename OperatorT, typename... Ts>
inline auto NodeEngine::createNode(Ts&&... ts) -> NodeHandle<OperatorT> {
  std::unique_ptr<OperatorT> node_ptr;
  if constexpr (std::is_constructible_v<OperatorT, NodeEngine&>) {
    node_ptr = std::make_unique<OperatorT>(*this);
  } else {
    node_ptr = std::make_unique<OperatorT>();
  }
  auto* node = node_ptr.get();
  nodes_.emplace_back(std::move(node_ptr));
  using OperationDataT = std::decay_t<decltype(node->data_)>::value_type;
  if constexpr (std::is_constructible_v<OperationDataT, NodeEngine&, Ts...>) {
    node->data_.emplace(*this, std::forward<Ts>(ts)...);
  } else {
    node->data_.emplace(std::forward<Ts>(ts)...);
  }

  // Late initialize special members. This is required for tow reasons:
  //  1. We don't want to impose a ctor taking the engine parameter on
  //     an  Operator
  //  2. The name might only be fully valid after the node is fully constructed.
  node->implicit_output_.emplace(node, "output");
  node->engine_ = this;
  node->registerInputs();

  registerImplicitOutput(*node);

  scope_.spawn(createNodeRunner(*node));
  return NodeHandle{ node };
}

template <typename Output>
inline void NodeEngine::registerOutput(Output& output) {
  remote_node_handler_.registerOutput(*this, output);
}

template <typename Node>
inline void NodeEngine::registerImplicitOutput(Node& node) {
  remote_node_handler_.registerImplicitOutput(*this, node);
}

inline auto NodeEngine::uponError() {
  return stdexec::upon_error([&]<typename Error>(Error error) noexcept {
    if constexpr (std::is_same_v<std::exception_ptr, std::decay_t<Error>>) {
      if (exception_) {
        try {
          std::rethrow_exception(exception_);
        } catch (std::exception& exception) {
          heph::log(heph::ERROR, "Overriding previous exception", "exception", exception.what());
        } catch (...) {
          heph::log(heph::ERROR, "Overriding previous exception", "exception", "unknown");
        }
      }
      exception_ = std::move(error);
    } else {
      exception_ = std::make_exception_ptr(std::runtime_error("Unknown error"));
    }
    context_.requestStop();
  });
}

template <typename Node>
inline auto NodeEngine::uponStopped(Node* node) {
  return stdexec::upon_stopped([this, node] {
    if (context_.stopRequested()) {
      return;
    }
    auto it = std::ranges::find_if(nodes_, [node](auto& other) { return other.get() == node; });
    if (it != nodes_.end()) {
      auto node_ptr = std::move(*it);
      // Remove connected inputs...
      nodes_.erase(it);
      for (it = nodes_.begin(); it != nodes_.end(); ++it) {
        it->get()->removeOutputConnection(node);
      }
    }
  });
}

template <typename Node>
inline auto NodeEngine::createNodeRunner(Node& node) {
  auto runner = heph::concurrency::repeatUntil([&node, this] {
                  return node.triggerExecute() | stdexec::then([this] { return context_.stopRequested(); });
                }) |
                uponError() | uponStopped(&node);
  return std::move(runner);
}

}  // namespace heph::conduit
