//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <exec/when_any.hpp>
#include <fmt/format.h>
#include <rfl/NamedTuple.hpp>
#include <rfl/to_view.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/internal/never_stop.h"
#include "hephaestus/conduit/node_base.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/stepper.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"

namespace heph::conduit {
namespace internal {
template <typename Node, typename Children, typename Config, std::size_t... Idx>
void constructChildren(const std::string& prefix, Node* node, Children& children, const Config& config,
                       std::index_sequence<Idx...> /*unused*/) {
  (rfl::get<Idx>(children)->initialize(prefix, node, *rfl::get<Idx>(config)), ...);
}

template <typename Trigger, typename Inputs, std::size_t... Idx>
auto spawnInputTriggers(Trigger trigger, SchedulerT scheduler, Inputs& inputs,
                        std::index_sequence<Idx...> /*unused*/) {
  return trigger(rfl::get<Idx>(inputs)->trigger(scheduler)...);
}

template <typename Outputs, std::size_t... Idx>
auto spawnOutputTriggers(Outputs& outputs, SchedulerT scheduler, std::index_sequence<Idx...> /*unused*/
) {
  return stdexec::when_all(rfl::get<Idx>(outputs)->trigger(scheduler)...);
}

template <typename NodeDescription>
struct NodeImpl : public NodeBase {
public:
  using InputsT = typename NodeDescription::Inputs;
  using OutputsT = typename NodeDescription::Outputs;
  using ChildrenT = typename NodeDescription::Children;
  using TriggerT = typename NodeDescription::Trigger;
  using StepperT = Stepper<NodeDescription>;

  explicit NodeImpl(std::string prefix, NodeBase* parent, StepperT stepper)
    : NodeBase(std::move(prefix), NodeDescription::NAME, parent), stepper(stepper) {
    const auto& children_config = stepper.childrenConfig();
    using ChildrenConfigT = std::decay_t<decltype(children_config)>;
    auto childrens_view = rfl::to_view(children);
    using ChildrensViewT = std::decay_t<decltype(childrens_view)>;
    if constexpr (!std::is_same_v<rfl::NamedTuple<>, ChildrensViewT>) {
      auto children_config_view = rfl::to_view(children_config);
      internal::constructChildren(
          this->prefix(), this, childrens_view.values(), children_config_view.values(),
          std::make_index_sequence<rfl::tuple_size_v<typename ChildrensViewT::Values>>{});
      // Alternative with C++26 and aggregate packs.
      // auto& [... child] = children;
      // const auto& [... child_stepper] = stepper.nodeConfig().stepper;
      //(child.emplace(this, child_stepper), ...);
    }
  }

  void enable() final {
    auto inputs_view = rfl::to_view(inputs);
    inputs_view.apply([](const auto& input) { input.value()->enable(); });
  }

  void disable() final {
    auto inputs_view = rfl::to_view(inputs);
    inputs_view.apply([](const auto& input) { input.value()->disable(); });
  }

  auto spawn(SchedulerT scheduler) {
    return stdexec::schedule(scheduler) | stdexec::let_value([this, scheduler]() {
             return concurrency::repeatUntil([this, scheduler]() {
               auto now = ClockT::now();
               if (trigger_start_time != ClockT::time_point{}) {
                 auto period_duration = now - trigger_start_time;
                 heph::telemetry::record(
                     [timestamp = trigger_start_time, period_duration, name = this->name()] {
                       return heph::telemetry::Metric {
                     .component = fmt::format("conduit{}", name), .tag = "node_timings",
                     .timestamp = timestamp,
                     .values = {
                       {
                           "tick_duration_microsec",
                           std::chrono::duration_cast<std::chrono::microseconds>(period_duration).count(),
                       },
                     },
                   };
                     });
               }
               trigger_start_time = now;
               return stdexec::continues_on(inputTrigger(scheduler), scheduler) |
                      stdexec::let_value([this, scheduler](auto... /*unused*/) {
                        execution_start_time = ClockT::now();
                        return stdexec::continues_on(
                                   stepper.step(this->prefix(), this->moduleName(), inputs, outputs),
                                   scheduler) |
                               stdexec::let_value([this, scheduler]() { return outputTrigger(scheduler); });
                      }) |
                      stdexec::then([this]() {
                        auto execute_duration = ClockT::now() - execution_start_time;
                        heph::telemetry::record(
                            [timestamp = execution_start_time, execute_duration, name = this->name()] {
                              return heph::telemetry::Metric {
                     .component = fmt::format("conduit{}", name), .tag = "node_timings",
                     .timestamp = timestamp,
                     .values = {
                       {
                           "execute_duration_microsec",
                           std::chrono::duration_cast<std::chrono::microseconds>(execute_duration).count(),
                       },
                     },
                   };
                            });
                        return false;
                      });
             });
           });
  }

  auto inputTrigger(SchedulerT scheduler) {
    return stdexec::schedule(scheduler) | stdexec::let_value([this, scheduler]() {
             auto inputs_view = rfl::to_view(inputs);
             using InputsViewT = std::decay_t<decltype(inputs_view)>;
             if constexpr (!std::is_same_v<rfl::NamedTuple<>, InputsViewT>) {
               return internal::spawnInputTriggers(
                   trigger, scheduler, inputs_view.values(),
                   std::make_index_sequence<rfl::tuple_size_v<typename InputsViewT::Values>>{});

             } else {
               return NeverStop{};
             }
           });
  }

  auto outputTrigger(SchedulerT scheduler) {
    auto outputs_view = rfl::to_view(outputs);
    using OutputsViewT = std::decay_t<decltype(outputs_view)>;
    if constexpr (!std::is_same_v<rfl::NamedTuple<>, OutputsViewT>) {
      return internal::spawnOutputTriggers(
          outputs_view.values(), scheduler,
          std::make_index_sequence<rfl::tuple_size_v<typename OutputsViewT::Values>>{});
    } else {
      return stdexec::just();
    }
  }
  StepperT stepper;
  InputsT inputs{};
  OutputsT outputs{};
  ChildrenT children{};
  TriggerT trigger{};

  ClockT::time_point trigger_start_time;
  ClockT::time_point execution_start_time;
};
}  // namespace internal

template <typename NodeDescription>
class Node {
public:
  using InputsT = typename NodeDescription::Inputs;
  using OutputsT = typename NodeDescription::Outputs;
  using ChildrenT = typename NodeDescription::Children;
  using TriggerT = typename NodeDescription::Trigger;
  using StepperT = Stepper<NodeDescription>;
  Node() = default;

  auto operator*() -> auto& {
    return handle_.value();
  }

  auto operator*() const -> const auto& {
    return handle_.value();
  }

  auto operator->() -> auto* {
    return &handle_.value();
  }

  auto operator->() const -> const auto* {
    return &handle_.value();
  }

  void initialize(std::string prefix, NodeBase* parent, NodeDescription::StepperT stepper) {
    handle_.emplace(std::move(prefix), parent, stepper);
  }

private:
  std::optional<internal::NodeImpl<NodeDescription>> handle_;
};

struct WhenAll {
  template <typename... Ts>
  auto operator()(Ts&&... ts) const {
    return stdexec::when_all(std::forward<Ts>(ts)...);
  }
};

struct WhenAny {
  template <typename... Ts>
  auto operator()(Ts&&... ts) const {
    return exec::when_any(std::forward<Ts>(ts)...);
  }
};

template <typename NodeDescription>
struct NodeDescriptionDefaults {
  using StepperT = Stepper<NodeDescription>;
  struct ChildrenConfig {};

  template <typename InputsT, typename OutputsT, typename ChildrenT>
  static void connect(InputsT& /*inputs*/, OutputsT& /*outputs*/, ChildrenT& /*children*/) {
  }

  struct Inputs {};
  struct Outputs {};
  struct Children {};
  using Trigger = WhenAll;
};

}  // namespace heph::conduit
