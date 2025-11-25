//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <rfl/to_view.hpp>

#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/output_base.h"
#include "hephaestus/conduit/partner_output.h"
#include "hephaestus/conduit/typed_input.h"

namespace heph::conduit {
struct GraphConfig {
  std::string prefix;
  std::vector<std::string> partners;
};

namespace internal {

struct NullVisitor {
  void operator()(auto& /*node*/) {
  }
};

template <typename NodeDescription, typename PreVisitor, typename PostVisitor = NullVisitor>
void traverse(Node<NodeDescription>& node, PreVisitor pre_visitor, PostVisitor post_visitor = {}) {
  pre_visitor(node);

  auto view = rfl::to_view(node->children);
  view.apply([&](const auto& child) { traverse(*child.value(), pre_visitor, post_visitor); });

  // Alternative with C++26 and aggregate packs.
  // auto& [... children] = node->children;
  // if constexpr (sizeof...(children) > 0) {
  //   (traverse(*children, visitor), ...);
  // }

  post_visitor(node);
}
}  // namespace internal

template <typename Stepper>
class Graph {
public:
  using StepperT = Stepper;
  using NodeDescriptionT = typename StepperT::NodeDescriptionT;

  template <typename... Ts>
  explicit Graph(GraphConfig config, Ts&&... ts)
    : stepper_(std::forward<Ts>(ts)...), config_(std::move(config)) {
    // Initialize all nodes, recurses into child nodes
    root_.initialize(config_.prefix, nullptr, stepper_);

    // Connect all nodes, recurses into child nodes, also sets paths for inputs and outputs.
    internal::traverse(root_, internal::NullVisitor{},
                       [this]<typename NodeDescription>(Node<NodeDescription>& node) {
                         auto inputs = rfl::to_view(node->inputs);
                         inputs.apply([&](const auto& input) { input.value()->setNode(*node); });
                         auto outputs = rfl::to_view(node->outputs);
                         outputs.apply([&](const auto& output) { output.value()->setNode(*node); });
                         NodeDescription::connect(node->inputs, node->outputs, node->children);
                         node->stepper.connect(node->inputs, node->outputs, node->children);
                       });
    internal::traverse(root_, internal::NullVisitor{},
                       [this]<typename NodeDescription>(Node<NodeDescription>& node) {
                         auto inputs = rfl::to_view(node->inputs);
                         inputs.apply([&](const auto& input) { registerInput(*input.value()); });
                         auto outputs = rfl::to_view(node->outputs);
                         outputs.apply([&](const auto& output) { registerOutput(*output.value()); });
                       });
  }

  auto stepper() -> auto& {
    return stepper_;
  }

  auto stepper() const -> const auto& {
    return stepper_;
  }

  auto root() -> auto& {
    return root_;
  }

  auto root() const -> const auto& {
    return root_;
  }

  auto partnerOutputs() -> const std::vector<PartnerOutputBase*>& {
    return partner_outputs_;
  }

  [[nodiscard]] auto inputs() const -> const std::vector<BasicInput*>& {
    return typed_inputs_;
  }

private:
  void registerInput(BasicInput& /*input*/) {
  }
  template <typename T>
  void registerInput(TypedInput<T>& input) {
    typed_inputs_.push_back(&input);
  }
  void registerOutput(OutputBase& /*output*/) {
  }
  template <typename T, std::size_t Capacity>
  void registerOutput(Output<T, Capacity>& output) {
    typed_outputs_.push_back(&output);
    for (const auto& partner : config_.partners) {
      auto partner_outputs = output.setPartner(config_.prefix, partner);
      partner_outputs_.insert(partner_outputs_.end(), partner_outputs.begin(), partner_outputs.end());
    }
  }

private:
  StepperT stepper_;
  Node<NodeDescriptionT> root_;
  GraphConfig config_;
  std::vector<BasicInput*> typed_inputs_;
  std::vector<OutputBase*> typed_outputs_;
  std::vector<PartnerOutputBase*> partner_outputs_;
};
}  // namespace heph::conduit
