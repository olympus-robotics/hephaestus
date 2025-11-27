//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/executor.h"

#include <atomic>
#include <exception>
#include <memory>
#include <regex>
#include <string>
#include <utility>

#include <stdexec/execution.hpp>

#include "hephaestus/conduit/node_base.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/error_handling/panic.h"

namespace heph::conduit {

namespace internal {
void Runner::StopCallback::operator()() const noexcept {
  self->context_.requestStop();
}
Runner::Runner(stdexec::inplace_stop_token stop_token, RunnerConfig config)
  : config_(std::move(config))
  , context_(config_.context_config)
  , stop_callback_(stop_token, StopCallback{ this }) {
  std::atomic<bool> started{ false };
  thread_ = std::thread{ [this, &started]() {
    context_.run([&started]() {
      started.store(true, std::memory_order_release);
      started.notify_all();
    });
  } };
  while (!started.load(std::memory_order_acquire)) {
    started.wait(false, std::memory_order_acquire);
  }
}
auto Runner::match(const std::string& name) const -> bool {
  return std::regex_match(name, std::regex(config_.selector));
}
auto Runner::scheduler() -> SchedulerT {
  return context_.scheduler();
}
void Runner::join() {
  thread_.join();
}
}  // namespace internal

Executor::~Executor() noexcept {
  requestStop();
  join();
}

void Executor::join() {
  stdexec::sync_wait(scope_.on_empty());
  stop_source_.request_stop();
  for (auto& runner : runners_) {
    runner->join();
  }
  runners_.clear();
  acceptor_.join();
  if (exception_) {
    std::exception_ptr exception;
    std::swap(exception, exception_);
    std::rethrow_exception(exception);
  }
}

void Executor::requestStop() {
  scope_.request_stop();
  acceptor_.requestStop();
}

Executor::Executor(const ExecutorConfig& config) : acceptor_(config.acceptor) {
  runners_.reserve(config.runners.size());
  for (const auto& runner_config : config.runners) {
    runners_.emplace_back(std::make_unique<internal::Runner>(stop_source_.get_token(), runner_config));
  }
}

auto Executor::getScheduler(NodeBase& node) -> SchedulerT {
  auto name = node.name();
  for (auto& runner : runners_) {
    if (runner->match(name)) {
      return runner->scheduler();
    }
  }
  heph::panic("Could not match node against any runner");
}
}  // namespace heph::conduit
