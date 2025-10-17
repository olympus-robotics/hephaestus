//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <memory>
#include <regex>
#include <thread>
#include <utility>
#include <vector>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/graph.h"
#include "hephaestus/conduit/scheduler.h"

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

  auto match(const std::string& name) const -> bool;

  auto scheduler() -> SchedulerT;

  void join();

private:
  RunnerConfig config_;
  heph::concurrency::Context context_;
  stdexec::inplace_stop_callback<StopCallback> stop_callback_;
  std::thread thread_;
};
}  // namespace internal

class Executor {
public:
  Executor() : Executor({ { .selector = ".*", .context_config = {} } }) {
  }
  explicit Executor(const std::vector<RunnerConfig>& configs);

  ~Executor() noexcept;

  Executor(const Executor&) = delete;
  Executor(Executor&&) = delete;
  auto operator=(const Executor&) -> Executor& = delete;
  auto operator=(Executor&&) -> Executor& = delete;

  template <typename Stepper>
  void spawn(Graph<Stepper>& graph) {
    scope_.spawn(spawnImpl(graph.root()) | stdexec::upon_error([&](const std::exception_ptr& error) mutable {
                   exception_ = error;
                   stop_source_.request_stop();
                 }) |
                 stdexec::upon_stopped([&]() { stop_source_.request_stop(); }));
  }

  void requestStop();
  void join();

private:
  auto getScheduler(NodeBase& node) -> SchedulerT;

  template <typename NodeDescription, std::size_t... Idx>
  auto spawnImpl(SchedulerT scheduler, Node<NodeDescription>& node, std::index_sequence<Idx...> /*unused*/)
      -> concurrency::AnySender<void> {
    return stdexec::when_all(node->spawn(scheduler), spawnImpl(boost::pfr::get<Idx>(node->children))...);
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

  static thread_local Executor* current;
};

}  // namespace heph::conduit
