//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exec/task.hpp>

#include "hephaestus/concurrency/any_sender.h"

namespace heph::conduit {
template <typename Stepper, typename NodeDescription>
concept stepper = requires(Stepper& stepper) { typename Stepper::NodeDescriptionT; } &&
                  std::is_same_v<typename Stepper::NodeDescriptionT, NodeDescription>;

namespace internal {
template <typename NodeDescription>
struct StepperVtable {
  using InputsT = typename NodeDescription::Inputs;
  using OutputsT = typename NodeDescription::Outputs;
  using ChildrenT = typename NodeDescription::Children;
  using ChildrenConfigT = typename NodeDescription::ChildrenConfig;
  void (*connect)(void*, InputsT&, OutputsT&, ChildrenT&);
  concurrency::AnySender<void> (*step)(void*, InputsT&, OutputsT&);
  ChildrenConfigT (*children_config)(const void*);
};

template <typename Stepper, typename NodeDescription = typename Stepper::NodeDescriptionT>
inline constexpr auto STEPPER_VTABLE = StepperVtable<NodeDescription>{
  .connect = [](void* ptr, NodeDescription::Inputs& inputs, NodeDescription::Outputs& outputs,
                NodeDescription::Children& children) -> void {
    static_cast<Stepper*>(ptr)->connect(inputs, outputs, children);
  },
  .step = [](void* ptr, NodeDescription::Inputs& inputs,
             NodeDescription::Outputs& outputs) -> concurrency::AnySender<void> {
    auto step_fun = [ptr, &inputs, &outputs]() { return static_cast<Stepper*>(ptr)->step(inputs, outputs); };
    using ResultT = decltype(step_fun());

    if constexpr (std::is_same_v<void, ResultT>) {
      return stdexec::just() | stdexec::then(std::move(step_fun));
    } else {
      return step_fun();
    }
  },
  .children_config = [](const void* ptr) -> NodeDescription::ChildrenConfig {
    return static_cast<const Stepper*>(ptr)->childrenConfig();
  },
};
}  // namespace internal

template <typename NodeDescription>
class Stepper {
public:
  using NodeDescriptionT = NodeDescription;
  using ChildrenConfigT = typename NodeDescriptionT::ChildrenConfig;

  template <stepper<NodeDescriptionT> StepperT>
    requires(!std::is_same_v<StepperT, Stepper>)
  Stepper(StepperT& stepper_impl) : ptr_(&stepper_impl), vtable_(&internal::STEPPER_VTABLE<StepperT>) {
  }

  template <stepper<NodeDescriptionT> StepperT>
    requires(!std::is_same_v<StepperT, Stepper>)
  Stepper(const StepperT& stepper_impl) : Stepper(const_cast<StepperT&>(stepper_impl)) {
  }

  void connect(NodeDescriptionT::Inputs& inputs, NodeDescriptionT::Outputs& outputs,
               NodeDescriptionT::Children& children) {
    vtable_->connect(ptr_, inputs, outputs, children);
  }

  auto step(NodeDescriptionT::Inputs& inputs, NodeDescriptionT::Outputs& outputs)
      -> concurrency::AnySender<void> {
    return vtable_->step(ptr_, inputs, outputs);
  }

  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    return vtable_->children_config(ptr_);
  }

private:
  void* ptr_;
  const internal::StepperVtable<NodeDescriptionT>* vtable_;
};

template <typename NodeDescription>
struct StepperDefaults {
  using NodeDescriptionT = NodeDescription;
  using InputsT = NodeDescriptionT::Inputs;
  using OutputsT = NodeDescriptionT::Outputs;
  using ChildrenT = NodeDescriptionT::Children;
  using ChildrenConfigT = NodeDescriptionT::ChildrenConfig;

  void connect(InputsT& /*inputs*/, OutputsT& /*outputs*/, ChildrenT& /*children*/) {
  }

  void step(InputsT& /*inputs*/, OutputsT& /*outputs*/) {
  }

  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    return {};
  }
};
}  // namespace heph::conduit
