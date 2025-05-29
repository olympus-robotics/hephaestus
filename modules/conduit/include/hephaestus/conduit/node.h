//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <variant>

#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node_engine.h"

namespace heph::conduit {
namespace detail {
struct Unused {};
template <typename InputT, typename T, std::size_t Depth>
class InputBase;
}  // namespace detail

template <typename OperationT, typename OperationDataT = detail::Unused>
class Node : public detail::NodeBase {
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

  auto data() const -> OperationDataT const& {
    return data_.value();
  }

  auto data() -> OperationDataT& {
    return data_.value();
  }

  [[nodiscard]] auto nodeName() const -> std::string final;

  [[nodiscard]] auto nodePeriod() -> std::chrono::nanoseconds final;

private:
  auto invokeOperation() {
    return [this]<typename... Ts>(Ts&&... ts) {
      detail::ExecutionStopWatch stop_watch{ this };
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
    return executeSender() | implicit_output_->propagate(engine());
  }

  template <typename Input>
  void registerInput(Input* input) {
    implicit_output_->registerInput(input);
  }

  auto operation() -> OperationT& {
    return static_cast<OperationT&>(*this);
  }

  auto operation() const -> OperationT const& {
    return static_cast<OperationT const&>(*this);
  }

  auto operationTrigger() {
    auto period_trigger = [&] {
      if constexpr (HAS_PERIOD) {
        return engine().scheduler().scheduleAfter(lastPeriodDuration());
      } else {
        return stdexec::just();
      }
    };
    auto node_trigger = [&] {
      if constexpr (HAS_TRIGGER_NULLARY) {
        return OperationT::trigger();
      } else if constexpr (HAS_TRIGGER_ARG) {
        return OperationT::trigger(operation());
      } else {
        static_assert(HAS_PERIOD,
                      "An Operation needs to have at least either a trigger or a period function");
        return stdexec::just();
      }
    };
    return engine().scheduler().schedule() | stdexec::let_value([period_trigger, node_trigger] {
             return stdexec::when_all(period_trigger(), node_trigger());
           });
  }

private:
  friend class NodeEngine;

  template <typename InputT, typename T, std::size_t Depth>
  friend class detail::InputBase;

  std::optional<detail::OutputConnections> implicit_output_;
  std::optional<OperationDataT> data_;
};

template <typename OperationT, typename OperationDataT>
inline auto Node<OperationT, OperationDataT>::nodePeriod() -> std::chrono::nanoseconds {
  if constexpr (HAS_PERIOD_NULLARY) {
    return OperationT::period();
  } else if constexpr (HAS_PERIOD_ARG) {
    return OperationT::period(operation());
  } else if constexpr (HAS_PERIOD_CONSTANT) {
    return OperationT::PERIOD;
  } else {
    return {};
  }
}

template <typename OperationT, typename OperationDataT>
inline auto Node<OperationT, OperationDataT>::nodeName() const -> std::string {
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

}  // namespace heph::conduit
