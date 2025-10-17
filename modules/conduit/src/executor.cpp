//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/executor.h"

#include <regex>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/utils/exception.h"

namespace heph::conduit {

namespace internal {
void Runner::StopCallback::operator()() const noexcept {
  self->context_.requestStop();
}
Runner::Runner(stdexec::inplace_stop_token stop_token, RunnerConfig config)
  : config_(std::move(config))
  , context_(config_.context_config)
  , stop_callback_(stop_token, StopCallback{ this })
  , thread_([this]() { context_.run(); }) {
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
  stop_source_.request_stop();
  join();
}

void Executor::join() {
  for (auto& runner : runners_) {
    runner->join();
  }
  runners_.clear();
  if (exception_) {
    std::exception_ptr exception;
    std::swap(exception, exception_);
    std::rethrow_exception(exception);
  }
}

void Executor::requestStop() {
  stop_source_.request_stop();
}
Executor::Executor(const std::vector<RunnerConfig>& configs) {
  runners_.reserve(configs.size());
  for (const auto& config : configs) {
    runners_.emplace_back(std::make_unique<internal::Runner>(stop_source_.get_token(), config));
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
  __builtin_unreachable();
}
}  // namespace heph::conduit
