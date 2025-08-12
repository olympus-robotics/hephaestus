//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/utils/utils.h"

namespace heph::conduit {
class NodeEngine;
auto scheduler(NodeEngine& engine) -> concurrency::Context::Scheduler;
}  // namespace heph::conduit
namespace heph::conduit::detail {
class NodeBase;

template <typename T>
auto extractResult(T& t) -> T* {
  return &t;
}
template <typename T>
auto extractResult(std::optional<T>& t) -> T* {
  if (!t.has_value()) {
    return nullptr;
  }
  return &t.value();
}

class OutputConnections {
  struct InputEntry {
    using SetValueT = InputState (*)(void*, void*);
    using NameT = std::string (*)(void*);
    void* ptr;
    SetValueT set_value;
    NameT name;
    detail::NodeBase* node;
    std::size_t generation;
  };

  using SchedulerT = concurrency::Context::Scheduler;

public:
  static constexpr std::string_view INPUT_OVERFLOW_WARNING =
      "Delaying Output operation because receiving input would overflow";

  OutputConnections(detail::NodeBase* node, std::string name);

  auto propagate(NodeEngine& engine);

  [[nodiscard]] auto name() const -> std::string;
  [[nodiscard]] auto name(const std::string& prefix) const -> std::string;

  template <typename Input>
  void registerInput(Input* input) {
    inputs_.emplace_back(
        input,
        [](void* input_ptr, void* ptr) {
          return static_cast<Input*>(input_ptr)->setValue(*static_cast<typename Input::ValueT*>(ptr));
        },
        [](void* input_ptr) { return std::string{ static_cast<Input*>(input_ptr)->name() }; }, input->node(),
        generation_);

    registerInputToEngine(input->name(), heph::utils::getTypeName<typename Input::ValueT>(), input->node());
  }

  void removeConnection(void* node) {
    while (true) {
      auto it = std::ranges::find_if(inputs_, [node](const InputEntry& entry) { return entry.node == node; });
      if (it == inputs_.end()) {
        return;
      }
      inputs_.erase(it);
    }
  }

private:
  void registerInputToEngine(const std::string& name, std::string type, detail::NodeBase* node);

  using ScheduleAfterResultT = std::decay_t<decltype(std::declval<SchedulerT>().scheduleAfter(
      std::declval<std::chrono::milliseconds>()))>;
  auto retryTimeout(NodeEngine& engine) -> ScheduleAfterResultT;

private:
  std::vector<InputEntry> inputs_;
  std::size_t generation_{ 0 };
  std::size_t retry_{ 0 };
  detail::NodeBase* node_;
  std::string name_;
};

inline auto OutputConnections::propagate(NodeEngine& engine) {
  return stdexec::let_value([this, &engine]<typename... Ts>(Ts&&... ts) {
    // If the continuation didn't get any parameters, the operator
    // return void, and we can move on ...
    // Otherwise, we attempt to set the result to connected inputs.
    if constexpr (sizeof...(Ts) == 1) {
      auto args = std::make_tuple(std::forward<Ts>(ts)...);
      return heph::concurrency::repeatUntil([this, &engine, args = std::move(args)]() {
        return retryTimeout(engine) | stdexec::then([this, args = std::move(args)] {
                 bool done = std::apply(
                     [this]<typename T>(T result) {
                       if (inputs_.empty()) {
                         return true;
                       }
                       //  Extract result: In case of an optional without a value, we
                       //  get a nullptr and return immediately without propagating
                       //  anything.
                       auto* result_ptr = extractResult(result);
                       if (result_ptr == nullptr) {
                         return true;
                       }

                       std::size_t propagated_count{ 0 };
                       for (InputEntry& entry : inputs_) {
                         if (entry.generation != generation_) {
                           ++propagated_count;
                           continue;
                         }
                         if (entry.set_value(entry.ptr, result_ptr) == InputState::OK) {
                           ++propagated_count;
                           ++entry.generation;
                         }
                       }
                       if (propagated_count == inputs_.size()) {
                         // Everything is propagated, now reset...
                         generation_++;
                         retry_ = 0;
                         return true;
                       }
                       ++retry_;
                       return false;
                     },
                     args);
                 return done;
               });
      });
    } else {
      (void)this;
      static_assert(sizeof...(Ts) == 0, "Node returning more than one output should be impossible");
      return stdexec::just();
    }
  });
}

inline auto OutputConnections::retryTimeout(NodeEngine& engine) -> ScheduleAfterResultT {
  // TODO: find better way to timeout based on the inputs timing...
  // Currently doing floor(retry^1.5)
  static constexpr float EXP = 1.5f;
  std::chrono::milliseconds timeout{ 0 };
  if (retry_ > 0) {
    timeout = std::chrono::milliseconds(
        static_cast<std::chrono::milliseconds::rep>(std::floor(std::pow(static_cast<float>(retry_), EXP))));

    auto remaining_inputs =
        inputs_ |
        std::views::filter([this](const InputEntry& entry) { return entry.generation <= generation_; }) |
        std::views::transform([](InputEntry& entry) { return entry.name(entry.ptr); });

    heph::log(heph::WARN, std::string(INPUT_OVERFLOW_WARNING), "output", name(), "inputs",
              fmt::format("[{}]", fmt::join(remaining_inputs, ", ")), "retry", retry_, "delay", timeout);
  }
  return heph::conduit::scheduler(engine).scheduleAfter(timeout);
}
}  // namespace heph::conduit::detail
