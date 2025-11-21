//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <exec/async_scope.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/conduit/acceptor.h"
#include "hephaestus/conduit/graph.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_base.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/net/endpoint.h"

namespace heph::conduit {
struct RunnerConfig {
  std::string selector;
  concurrency::ContextConfig context_config;
};
namespace internal {
class Runner {
  struct StopCallback {
    void operator()() const noexcept;
    Runner* self;
  };

public:
  explicit Runner(stdexec::inplace_stop_token stop_token, RunnerConfig config);

  [[nodiscard]] auto match(const std::string& name) const -> bool;

  auto scheduler() -> SchedulerT;

  void join();

private:
  RunnerConfig config_;
  heph::concurrency::Context context_;
  stdexec::inplace_stop_callback<StopCallback> stop_callback_;
  std::thread thread_;
};
}  // namespace internal

struct ExecutorConfig {
  std::vector<RunnerConfig> runners;
  AcceptorConfig acceptor;
};

class Executor {
  struct Env {
    Executor* self;
    [[nodiscard]] auto query(stdexec::get_stop_token_t /*ignore*/) const noexcept
        -> stdexec::inplace_stop_token {
      return self->stop_source_.get_token();
    }
  };

public:
  Executor()
    : Executor({
          .runners = { {
              .selector = ".*",
              .context_config = {},
          }, },
          .acceptor = {},
      }) {
  }
  explicit Executor(const ExecutorConfig& config);

  ~Executor() noexcept;

  Executor(const Executor&) = delete;
  Executor(Executor&&) = delete;
  auto operator=(const Executor&) -> Executor& = delete;
  auto operator=(Executor&&) -> Executor& = delete;

  template <typename Stepper>
  void spawn(Graph<Stepper>& graph) {
    scope_.spawn(spawnImpl(graph.root()) | stdexec::upon_error([&](const std::exception_ptr& error) mutable {
                   try {
                     std::rethrow_exception(error);
                   } catch (std::exception& e) {
                     heph::panic("Executor::spawn exception: {}", e.what());
                   } catch (...) {
                     heph::panic("Executor::spawn unknown exception");
                   }
                 }));
    acceptor_.setInputs(graph.inputs());
    acceptor_.spawn(graph.partnerOutputs());
  }

  void requestStop();
  void join();

  void addPartner(const std::string& name, const heph::net::Endpoint& endpoint) {
    acceptor_.addPartner(name, endpoint);
  }

  [[nodiscard]] auto endpoints() const -> std::vector<heph::net::Endpoint> {
    return acceptor_.endpoints();
  }

  auto scheduler() -> SchedulerT {
    return runners_.front()->scheduler();
  }

private:
  auto getScheduler(NodeBase& node) -> SchedulerT;

  template <typename NodeDescription, std::size_t... Idx>
  auto spawnImpl(SchedulerT scheduler, Node<NodeDescription>& node, std::index_sequence<Idx...> /*unused*/)
      -> concurrency::AnySender<void> {
    return stdexec::continues_on(
        stdexec::when_all(node->spawn(scheduler), spawnImpl(boost::pfr::get<Idx>(node->children))...),
        scheduler);
  }

  template <typename NodeDescription>
  auto spawnImpl(Node<NodeDescription>& node) -> concurrency::AnySender<void> {
    using ChildrenT = NodeDescription::Children;
    return spawnImpl(getScheduler(*node), node,
                     std::make_index_sequence<boost::pfr::tuple_size_v<ChildrenT>>{});
  }

private:
  exec::async_scope scope_;
  std::exception_ptr exception_;
  stdexec::inplace_stop_source stop_source_;
  std::vector<std::unique_ptr<internal::Runner>> runners_;
  Acceptor acceptor_;

  static thread_local Executor* current;
};

}  // namespace heph::conduit
