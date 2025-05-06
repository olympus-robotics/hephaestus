//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <type_traits>

#include <exec/async_scope.hpp>
#include <exec/repeat_effect_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/telemetry/log.h"

namespace heph::conduit {
struct NodeEngineConfig {
  heph::concurrency::ContextConfig context_config;
  std::uint32_t number_of_threads{ 1 };
};

class NodeEngine {
public:
  explicit NodeEngine(NodeEngineConfig config);

  template <typename Node>
  void addNode(Node& node) {
    scope_.spawn(createNodeRunner(node));
  }

  void run();
  void requestStop();

  auto scheduler() {
    return context_.scheduler();
  }

  auto poolScheduler() {
    return pool_.get_scheduler();
  }

  auto elapsed() {
    return context_.elapsed();
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
  auto runner = exec::repeat_effect_until(node.execute(*this) |
                                          stdexec::then([this] { return context_.stopRequested(); })) |
                uponError();
  return std::move(runner);
}
}  // namespace heph::conduit
