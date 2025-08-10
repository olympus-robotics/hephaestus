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

namespace heph::conduit {
namespace detail {
struct Unused {};
template <typename InputT, typename T, std::size_t Depth>
class InputBase;
}  // namespace detail

class NodeEngine;

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
  static constexpr bool HAS_TRIGGER_ARG_PTR = requires(OperationT& op) { OperationT::trigger(&op); };

  template <typename... Ts>
  static constexpr bool HAS_EXECUTE_NULLARY =
      requires(OperationT&, Ts&&... ts) { OperationT::execute(std::forward<Ts>(ts)...); };
  template <typename... Ts>
  static constexpr bool HAS_EXECUTE_ARG =
      requires(OperationT& op, Ts&&... ts) { OperationT::execute(op, std::forward<Ts>(ts)...); };
  template <typename... Ts>
  static constexpr bool HAS_EXECUTE_ARG_PTR =
      requires(OperationT& op, Ts&&... ts) { OperationT::execute(&op, std::forward<Ts>(ts)...); };

public:
  static constexpr bool HAS_PERIOD = HAS_PERIOD_CONSTANT || HAS_PERIOD_NULLARY || HAS_PERIOD_ARG;
  static constexpr bool HAS_NAME = HAS_NAME_CONSTANT || HAS_NAME_NULLARY || HAS_NAME_ARG;

  auto data() const -> const OperationDataT& {
    return data_.value();
  }

  auto data() -> OperationDataT& {
    return data_.value();
  }

  [[nodiscard]] auto nodeName() const -> std::string final;

  [[nodiscard]] auto nodePeriod() -> std::chrono::nanoseconds final;

  void removeOutputConnection(void* node) final {
    implicit_output_->removeConnection(node);
  }

private:
  auto invokeOperation() {
    return [this]<typename... Ts>(Ts&&... ts) {
      detail::ExecutionStopWatch stop_watch{ this };
      static_assert(HAS_EXECUTE_ARG_PTR<Ts...> || HAS_EXECUTE_ARG<Ts...> || HAS_EXECUTE_NULLARY<Ts...>,
                    "No valid execute function available");
      if constexpr (HAS_EXECUTE_ARG<Ts...>) {
        return OperationT::execute(operation(), std::forward<Ts>(ts)...);
      } else if constexpr (HAS_EXECUTE_ARG_PTR<Ts...>) {
        return OperationT::execute(&operation(), std::forward<Ts>(ts)...);
      } else {
        return OperationT::execute(std::forward<Ts>(ts)...);
      }
    };
  }

  auto executeSender() {
    auto invoke_operation = invokeOperation();

    auto trigger = stdexec::continues_on(operationTrigger(), heph::conduit::scheduler(engine()));
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
    return executeSender() | implicit_output_->propagate(engine()) |
           stdexec::then([this] { operationEnd(); });
  }

  template <typename Input>
  void registerInput(Input* input) {
    implicit_output_->registerInput(input);
  }

  auto operation() -> OperationT& {
    return static_cast<OperationT&>(*this);
  }

  auto operation() const -> const OperationT& {
    return static_cast<const OperationT&>(*this);
  }

  auto operationTrigger() {
    auto period_trigger = [this](detail::NodeBase::ClockT::time_point start_at) {
      if constexpr (HAS_PERIOD) {
        return heph::conduit::scheduler(engine()).scheduleAt(start_at);
      } else {
        return stdexec::just();
      }
    };
    auto node_trigger = [this] {
      if constexpr (HAS_TRIGGER_NULLARY) {
        return OperationT::trigger();
      } else if constexpr (HAS_TRIGGER_ARG) {
        return OperationT::trigger(operation());
      } else if constexpr (HAS_TRIGGER_ARG_PTR) {
        return OperationT::trigger(&operation());
      } else {
        static_assert(HAS_PERIOD,
                      "An Operation needs to have at least either a trigger or a period function");
        return stdexec::just();
      }
    };
    return stdexec::just() | stdexec::let_value([this, period_trigger] {
             auto start_at = operationStart(HAS_PERIOD);
             return period_trigger(start_at);
           }) |
           stdexec::let_value([node_trigger] { return node_trigger(); });
  }

private:
  friend class NodeEngine;
  friend class RemoteNodeHandler;

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
