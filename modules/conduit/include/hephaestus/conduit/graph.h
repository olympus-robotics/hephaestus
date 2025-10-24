//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <string>
#include <utility>

#include <boost/pfr.hpp>
#include <exec/async_scope.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <hephaestus/concurrency/context.h>
#include <hephaestus/concurrency/repeat_until.h>

#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/output_base.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/net/endpoint.h"

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

  boost::pfr::for_each_field(node->children,
                             [&](auto& child) { traverse(child, pre_visitor, post_visitor); });

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

  explicit Graph(GraphConfig config, StepperT stepper)
    : stepper_(std::move(stepper)), config_(std::move(config)) {
    // Initialize all nodes, recurses into child nodes
    root_.initialize(config_.prefix, nullptr, stepper_);

    // Connect all nodes, recurses into child nodes, also sets paths for inputs and outputs.
    internal::traverse(
        root_, internal::NullVisitor{}, [this]<typename NodeDescription>(Node<NodeDescription>& node) {
          boost::pfr::for_each_field(node->inputs, [&node, this](auto& input) { input.setNode(*node); });
          boost::pfr::for_each_field(node->outputs, [&node, this](auto& output) { output.setNode(*node); });
          NodeDescription::connect(node->inputs, node->outputs, node->children);
          node->stepper.connect(node->inputs, node->outputs, node->children);
        });
    internal::traverse(
        root_, internal::NullVisitor{}, [this]<typename NodeDescription>(Node<NodeDescription>& node) {
          boost::pfr::for_each_field(node->inputs, [&node, this](auto& input) { registerInput(input); });
          boost::pfr::for_each_field(node->outputs, [&node, this](auto& output) { registerOutput(output); });
        });
  }

  template <typename Self>
  auto stepper(this Self&& self) -> auto& {
    return std::forward<Self>(self).stepper_;
  }
  template <typename Self>
  auto root(this Self&& self) -> auto& {
    return std::forward<Self>(self).root_;
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
  void registerOutput(OutputBase& /*input*/) {
  }
  template <typename T>
  void registerOutput(Output<T>& output) {
    typed_outputs_.push_back(&output);
    for (const auto& partner : config_.partners) {
      auto partner_outputs = output.setPartner(partner);
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

#if 0
namespace internal {}  // namespace internal

template <typename Stepper>
void run(heph::concurrency::Context& context, Graph<Stepper>& graph) {
  exec::async_scope scope;

  std::exception_ptr exception;

  scope.spawn(internal::spawn(context.scheduler(), graph.root()) |
              stdexec::upon_error([&](const std::exception_ptr& error) mutable {
                exception = error;
                scope.request_stop();
                context.requestStop();
              }) |
              stdexec::upon_stopped([&]() {
                scope.request_stop();
                context.requestStop();
              }));

  context.run();
  scope.request_stop();
  stdexec::sync_wait(scope.on_empty());

  if (exception) {
    std::rethrow_exception(exception);
  }
}
#endif
}  // namespace heph::conduit
