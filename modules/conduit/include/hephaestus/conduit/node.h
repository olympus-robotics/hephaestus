//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <type_traits>
#include <variant>

#include <hephaestus/telemetry/log_sink.h>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log.h"

namespace heph::conduit {
template <typename OperationT>
class Node {
public:
  static constexpr bool HAS_PERIOD = requires(OperationT& op) { op.period(); };

  auto name() const {
    return operation().name();
  }

  auto execute(NodeEngine& engine) {
    auto invoke_operation = [this, &engine]<typename... Ts>(Ts&&... ts) {
      ExecutionStopWatch stop_watch{ this };
      if constexpr (std::is_invocable_v<OperationT&, NodeEngine&, Ts...>) {
        return operation()(engine, std::forward<Ts>(ts)...);
      } else {
        (void)engine;
        return operation()(std::forward<Ts>(ts)...);
      }
    };

    auto trigger = operationTrigger(engine);
    using trigger_type = decltype(trigger);
    using trigger_values_variant = stdexec::value_types_of_t<trigger_type>;
    static_assert(std::variant_size_v<trigger_values_variant> == 1);
    using trigger_values = std::variant_alternative_t<0, trigger_values_variant>;
    using result_type = decltype(std::apply(invoke_operation, std::declval<trigger_values>()));
    return std::move(trigger) | [&invoke_operation] {
      if constexpr (stdexec::sender<result_type>) {
        return stdexec::let_value(invoke_operation);
      } else {
        return stdexec::then(invoke_operation);
      }
    }();  // | outputs_.propagate(engine);
  }

  auto operation() -> OperationT& {
    return static_cast<OperationT&>(*this);
  }

private:
  auto operationTrigger(NodeEngine& engine) {
    auto schedule_trigger = [&] {
      if constexpr (HAS_PERIOD) {
        auto period = operation().period();
        std::chrono::nanoseconds start_after = period - last_execution_duration_;
        // We attempt to avoid drift by adapting the expiry time with the
        // measured execution time of the last execution duration.
        // If execution took longer, we should scheduler immediately (delay of 0).
        if (last_execution_duration_ >= period) {
          heph::log(heph::WARN, "Missed deadline", "period", period, "duration", last_execution_duration_);
          start_after = std::chrono::nanoseconds{ 0 };
        }
        return engine.scheduler().scheduleAfter(start_after);
      } else {
        return engine.scheduler().schedule();
      }
    };
    return stdexec::just() | stdexec::let_value(schedule_trigger) | stdexec::let_value([this, &engine] {
             if constexpr (requires(OperationT& op) { op.trigger(); }) {
               return operation().trigger();
             } else if constexpr (requires(OperationT& op) { op.trigger(engine); }) {
               return operation().trigger(engine);
             } else {
               static_assert(HAS_PERIOD,
                             "An Operation needs to have at least either a trigger or a period function");
               return stdexec::just();
             }
           });
  }

  void updateExecutionTime(std::chrono::nanoseconds duration) noexcept {
    last_execution_duration_ = duration;
  }

private:
  class ExecutionStopWatch {
  public:
    explicit ExecutionStopWatch(Node* self) : self_(self), start_(std::chrono::high_resolution_clock::now()) {
    }
    ~ExecutionStopWatch() noexcept {
      auto end = std::chrono::high_resolution_clock::now();
      self_->updateExecutionTime(end - start_);
    }

    ExecutionStopWatch(ExecutionStopWatch const&) = delete;
    auto operator=(ExecutionStopWatch const&) -> ExecutionStopWatch& = delete;
    ExecutionStopWatch(ExecutionStopWatch&&) = delete;
    auto operator=(ExecutionStopWatch&&) -> ExecutionStopWatch& = delete;

  private:
    Node* self_;
    std::chrono::high_resolution_clock::time_point start_;
  };

  detail::OutputConnections outputs_;
  std::chrono::nanoseconds last_execution_duration_;
};
}  // namespace heph::conduit
