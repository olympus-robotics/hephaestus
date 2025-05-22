//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"

namespace heph::conduit {
namespace detail {
struct Unused {};
template <typename InputT, typename T, std::size_t Depth>
class InputBase;
}  // namespace detail

template <typename OperationT, typename OperationDataT = detail::Unused>
class Node : detail::NodeBase {
  static constexpr bool HAS_PERIOD_CONSTANT = requires(OperationT&) { OperationT::PERIOD; };
  static constexpr bool HAS_PERIOD_NULLARY = requires(OperationT&) { OperationT::period(); };
  static constexpr bool HAS_PERIOD_ARG = requires(OperationT& op) { OperationT::period(op); };
  static constexpr bool HAS_NAME_CONSTANT = requires(OperationT&) { OperationT::NAME; };
  static constexpr bool HAS_NAME_NULLARY = requires(OperationT&) { OperationT::name(); };
  static constexpr bool HAS_NAME_ARG = requires(OperationT& op) { OperationT::name(op); };
  static constexpr bool HAS_TRIGGER_NULLARY = requires(OperationT&) { OperationT::trigger(); };
  static constexpr bool HAS_TRIGGER_ARG = requires(OperationT& op) { OperationT::trigger(op); };

  template <typename... Ts>
  static constexpr bool HAS_EXECUTE_NULLARY =
      requires(OperationT&, Ts&&... ts) { OperationT::execute(std::forward<Ts>(ts)...); };
  template <typename... Ts>
  static constexpr bool HAS_EXECUTE_ARG =
      requires(OperationT& op, Ts&&... ts) { OperationT::execute(op, std::forward<Ts>(ts)...); };

public:
  static constexpr bool HAS_PERIOD = HAS_PERIOD_CONSTANT || HAS_PERIOD_NULLARY || HAS_PERIOD_ARG;
  static constexpr bool HAS_NAME = HAS_NAME_CONSTANT || HAS_NAME_NULLARY || HAS_NAME_ARG;
  static constexpr std::string_view MISSED_DEADLINE_WARNING = "Missed deadline";

  auto engine() -> NodeEngine& {
    return *engine_;
  }

  [[nodiscard]] auto engine() const -> NodeEngine const& {
    return *engine_;
  }

  auto data() const -> OperationDataT const& {
    return data_.value();
  }

  auto data() -> OperationDataT& {
    return data_.value();
  }

  [[nodiscard]] auto nodeName() const -> std::string final {
    if constexpr (HAS_NAME_ARG) {
      return std::string{ OperationT::name(operation()) };
    } else if constexpr (HAS_NAME_NULLARY) {
      return std::string{ OperationT::name() };
    } else if constexpr (HAS_NAME_CONSTANT) {
      return std::string{ OperationT::NAME };
    } else {
      return std::string{ typeid(OperationT).name() };
    }
  }

private:
  auto invokeOperation() {
    return [this]<typename... Ts>(Ts&&... ts) {
      ExecutionStopWatch stop_watch{ this };
      static_assert(HAS_EXECUTE_ARG<Ts...> || HAS_EXECUTE_NULLARY<Ts...>,
                    "No valid execute function available");
      if constexpr (HAS_EXECUTE_ARG<Ts...>) {
        return OperationT::execute(operation(), std::forward<Ts>(ts)...);
      } else {
        return OperationT::execute(std::forward<Ts>(ts)...);
      }
    };
  }

  auto executeSender() {
    auto invoke_operation = invokeOperation();

    auto trigger = operationTrigger();
    using TriggerT = decltype(trigger);
    using TriggerValuesVariantT = stdexec::value_types_of_t<TriggerT>;
    // static_assert(std::variant_size_v<TriggerValuesVariantT> == 1);
    using TriggerValuesT = std::variant_alternative_t<0, TriggerValuesVariantT>;
    using ResultT = decltype(std::apply(invoke_operation, std::declval<TriggerValuesT>()));
    return std::move(trigger) | [&invoke_operation] {
      if constexpr (stdexec::sender<ResultT>) {
        return stdexec::let_value(invoke_operation);
      } else {
        return stdexec::then(invoke_operation);
      }
    }();
  }

  auto triggerExecute() {
    return executeSender() | outputs_->propagate(*engine_);
  }

  template <typename Input>
  void registerInput(Input* input) {
    outputs_->registerInput(input);
  }

  auto operation() -> OperationT& {
    return static_cast<OperationT&>(*this);
  }

  auto operation() const -> OperationT const& {
    return static_cast<OperationT const&>(*this);
  }

  auto operationTrigger() {
    auto schedule_trigger = [&] {
      if constexpr (HAS_PERIOD) {
        auto period = [this]() {
          if constexpr (HAS_PERIOD_NULLARY) {
            return OperationT::period();
          } else if constexpr (HAS_PERIOD_ARG) {
            return OperationT::period(operation());
          } else {
            return OperationT::PERIOD;
          }
        }();
        std::chrono::nanoseconds start_after = period - last_execution_duration_;
        // We attempt to avoid drift by adapting the expiry time with the
        // measured execution time of the last execution duration.
        // If execution took longer, we should scheduler immediately (delay of 0).
        if (last_execution_duration_ >= period) {
          heph::log(heph::WARN, std::string{ MISSED_DEADLINE_WARNING }, "node", nodeName(), "period", period,
                    "duration", last_execution_duration_);
          start_after = std::chrono::nanoseconds{ 0 };
        }
        return engine_->scheduler().scheduleAfter(start_after);
      } else {
        return engine_->scheduler().schedule();
      }
    };
    return stdexec::just() | stdexec::let_value(schedule_trigger) | stdexec::let_value([this] {
             if constexpr (HAS_TRIGGER_NULLARY) {
               return OperationT::trigger();
             } else if constexpr (HAS_TRIGGER_ARG) {
               return OperationT::trigger(operation());
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
  friend class NodeEngine;

  template <typename InputT, typename T, std::size_t Depth>
  friend class detail::InputBase;

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

  std::optional<detail::OutputConnections> outputs_;
  std::chrono::nanoseconds last_execution_duration_{};
  NodeEngine* engine_{ nullptr };
  std::optional<OperationDataT> data_;
};
}  // namespace heph::conduit
