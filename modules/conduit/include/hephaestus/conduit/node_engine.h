//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/repeat_effect_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/node_handle.h"
#include "hephaestus/telemetry/log.h"

namespace heph::conduit {
struct NodeEngineConfig {
  heph::concurrency::ContextConfig context_config;
  std::uint32_t number_of_threads{ 1 };
};

class NodeEngine {
public:
  explicit NodeEngine(NodeEngineConfig config);

  void run();
  void requestStop();

  auto getStopToken() {
    return context_.getStopToken();
  }

  auto scheduler() {
    return context_.scheduler();
  }

  auto poolScheduler() {
    return pool_.get_scheduler();
  }

  auto elapsed() {
    return context_.elapsed();
  }

  template <typename OperatorT, typename... Ts>
  auto createNode(Ts&&... ts) -> NodeHandle<OperatorT> {
    std::unique_ptr<OperatorT> node_ptr;
    if constexpr (std::is_constructible_v<OperatorT, NodeEngine&>) {
      node_ptr = std::make_unique<OperatorT>(*this);
    } else {
      node_ptr = std::make_unique<OperatorT>();
    }
    auto* node = node_ptr.get();
    nodes_.emplace_back(std::move(node_ptr));
    node->data_.emplace(std::forward<Ts>(ts)...);
    // Late initialize special members. This is required for tow reasons:
    //  1. We don't want to impose a ctor taking the engine parameter on
    //     an  Operator
    //  2. The name might only be fully valid after the node is fully constructed.
    node->implicit_output_.emplace(node->nodeName());
    node->engine_ = this;
    scope_.spawn(createNodeRunner(*node));
    return NodeHandle{ node };
  }

private:
  auto uponError();

  template <typename Node>
  auto createNodeRunner(Node& node);

private:  // Optimal fields order: pool_, exception_, scope_, context_
  exec::static_thread_pool pool_;
  std::exception_ptr exception_;
  exec::async_scope scope_;
  heph::concurrency::Context context_{ {} };

  std::vector<std::unique_ptr<detail::NodeBase>> nodes_;
};

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
inline auto NodeEngine::createNodeRunner(Node& node) {
  auto runner = exec::repeat_effect_until(node.triggerExecute() |
                                          stdexec::then([this] { return context_.stopRequested(); })) |
                uponError();
  return std::move(runner);
}
}  // namespace heph::conduit
