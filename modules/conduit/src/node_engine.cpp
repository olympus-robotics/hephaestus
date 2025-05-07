//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/node_engine.h"

#include <exception>

#include <stdexec/execution.hpp>

namespace heph::conduit {

NodeEngine::NodeEngine(NodeEngineConfig config)
  : pool_(config.number_of_threads), context_(config.context_config) {
}
void NodeEngine::run() {
  context_.run();
  if (exception_) {
    std::rethrow_exception(exception_);
  }
  stdexec::sync_wait(scope_.on_empty());
}
void NodeEngine::requestStop() {
  context_.requestStop();
  pool_.request_stop();
}
}  // namespace heph::conduit
