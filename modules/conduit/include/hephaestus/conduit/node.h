//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <optional>
#include <utility>

#include <boost/pfr.hpp>
#include <exec/when_any.hpp>
#include <hephaestus/utils/exception.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/internal/never_stop.h"
#include "hephaestus/conduit/node_base.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/stepper.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry/metric_sink.h"

namespace heph::conduit {
namespace internal {
template <typename Node, typename Children, typename Config, std::size_t... Idx>
void constructChildren(const std::string& prefix, Node* node, Children& children, const Config& config,
                       std::index_sequence<Idx...> /*unused*/) {
  (boost::pfr::get<Idx>(children).initialize(prefix, node, boost::pfr::get<Idx>(config)), ...);
}

template <typename Trigger, typename Inputs, std::size_t... Idx>
auto spawnInputTriggers(Trigger trigger, SchedulerT scheduler, Inputs& inputs,
                        std::index_sequence<Idx...> /*unused*/) {
  if constexpr (sizeof...(Idx) == 0) {
    return NeverStop{};
  } else {
    return trigger(boost::pfr::get<Idx>(inputs).trigger(scheduler)...);
  }
}

template <typename Outputs, std::size_t... Idx>
auto spawnOutputTriggers(Outputs& outputs, std::index_sequence<Idx...> /*unused*/
) {
  if constexpr (sizeof...(Idx) == 0) {
    return stdexec::just();
  } else {
    return stdexec::when_all(boost::pfr::get<Idx>(outputs).trigger()...);
  }
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
    : prefix(std::move(prefix)), parent(parent), stepper(stepper) {
    const auto& children_config = stepper.childrenConfig();
    using ChildrenConfigT = std::decay_t<decltype(children_config)>;
    static_assert(boost::pfr::tuple_size_v<ChildrenT> == boost::pfr::tuple_size_v<ChildrenConfigT>);
    internal::constructChildren(this->prefix, this, children, children_config,
                                std::make_index_sequence<boost::pfr::tuple_size_v<ChildrenT>>{});
    // Alternative with C++26 and aggregate packs.
    // auto& [... child] = children;
    // const auto& [... child_stepper] = stepper.nodeConfig().stepper;
    //(child.emplace(this, child_stepper), ...);
  }

  [[nodiscard]] auto name() const -> std::string final {
    std::string res = fmt::format("/{}/", prefix);
    if (parent != nullptr) {
      res = fmt::format("{}/", parent->name());
    }
    return res + std::string{ NodeDescription::NAME };
  }

  void enable() final {
    boost::pfr::for_each_field(inputs, [](auto& input) { input.enable(); });
  }

  void disable() final {
    boost::pfr::for_each_field(inputs, [](auto& input) { input.disable(); });
  }

  auto spawn(SchedulerT scheduler) {
    return stdexec::schedule(scheduler) | stdexec::let_value([this, scheduler]() {
             return concurrency::repeatUntil([this, scheduler]() {
               auto now = ClockT::now();
               if (trigger_start_time != ClockT::time_point{}) {
                 auto period_duration = now - trigger_start_time;
                 heph::telemetry::record([name = name(), timestamp = trigger_start_time, period_duration] {
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
                        std::string module_name;
                        if (prefix.empty()) {
                          module_name = name();
                        } else {
                          module_name = name().substr(prefix.size() + 2);
                        }
                        return stdexec::continues_on(
                                   stepper.step(prefix, std::move(module_name), inputs, outputs), scheduler) |
                               stdexec::let_value([this, scheduler]() { return outputTrigger(); });
                      }) |
                      stdexec::then([this]() {
                        auto execute_duration = ClockT::now() - execution_start_time;
                        heph::telemetry::record(
                            [name = name(), timestamp = execution_start_time, execute_duration] {
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
             return internal::spawnInputTriggers(
                 trigger, scheduler, inputs, std::make_index_sequence<boost::pfr::tuple_size_v<InputsT>>{});
           });
  }

  auto outputTrigger() {
    return internal::spawnOutputTriggers(outputs,
                                         std::make_index_sequence<boost::pfr::tuple_size_v<OutputsT>>{});
  }

  std::string prefix;
  NodeBase* parent{ nullptr };
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
  static void connect(InputsT& /*inputs*/, OutputsT& /*outputs*/, ChildrenT& /*chdilren*/) {
  }

  struct Inputs {};
  struct Outputs {};
  struct Children {};
  using Trigger = WhenAll;
};

}  // namespace heph::conduit
