//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>
#include <fmt/format.h>
#include <fmt/std.h>
#include <gtest/gtest.h>

#include "hephaestus/conduit/stepper.h"

namespace heph::conduit {

struct DummyNodeDescription {
  struct Inputs {
    int value{ 0 };
  };
  struct Outputs {
    int value{ 1 };
  };
  struct Children {
    int value{ 2 };
  };
  struct ChildrenConfig {
    int value{ 3 };
  };
};
struct DummyNode {
  DummyNodeDescription::Inputs inputs;
  DummyNodeDescription::Outputs outputs;
  DummyNodeDescription::Children children;
};

struct DummyStepper : StepperDefaults<DummyNodeDescription> {
  bool connect_called{ false };
  bool step_called{ false };
  mutable bool children_config_called{ false };

  void connect(InputsT& inputs, OutputsT& outputs, ChildrenT& children) {
    connect_called = true;
    EXPECT_EQ(inputs.value, 0);
    EXPECT_EQ(outputs.value, 1);
    EXPECT_EQ(children.value, 2);
  }

  void step(InputsT& inputs, OutputsT& outputs) {
    step_called = true;
    EXPECT_EQ(inputs.value, 0);
    EXPECT_EQ(outputs.value, 1);
  }

  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    children_config_called = true;
    return {};
  }
};

TEST(Stepper, Interface) {
  DummyNode dummy{};
  DummyStepper dummy_stepper;
  Stepper<DummyNodeDescription> stepper{ dummy_stepper };

  std::ignore = stepper.childrenConfig();
  EXPECT_TRUE(dummy_stepper.children_config_called);

  stepper.connect(dummy.inputs, dummy.outputs, dummy.children);
  EXPECT_TRUE(dummy_stepper.connect_called);

  stdexec::sync_wait(stepper.step("", "", dummy.inputs, dummy.outputs));
  EXPECT_TRUE(dummy_stepper.step_called);
}

struct DummyStepperSender : StepperDefaults<DummyNodeDescription> {
  bool step_called{ false };

  auto step(InputsT& inputs, OutputsT& outputs) {
    return stdexec::just() | stdexec::then([this, &inputs, &outputs]() {
             EXPECT_EQ(inputs.value, 0);
             EXPECT_EQ(outputs.value, 1);
             step_called = true;
           });
  }
};

TEST(Stepper, InterfaceSender) {
  DummyNode dummy{};
  DummyStepperSender dummy_stepper;
  Stepper<DummyNodeDescription> stepper{ dummy_stepper };

  stdexec::sync_wait(stepper.step("", "", dummy.inputs, dummy.outputs));
  EXPECT_TRUE(dummy_stepper.step_called);
}

struct DummyStepperCoroutine : StepperDefaults<DummyNodeDescription> {
  bool step_called{ false };

  auto step(InputsT& inputs, OutputsT& outputs) -> exec::task<void> {
    step_called = true;
    EXPECT_EQ(inputs.value, 0);
    EXPECT_EQ(outputs.value, 1);
    co_return;
  }
};

TEST(Stepper, InterfaceCoroutine) {
  DummyNode dummy{};
  DummyStepperSender dummy_stepper;
  Stepper<DummyNodeDescription> stepper{ dummy_stepper };

  stdexec::sync_wait(stepper.step("", "", dummy.inputs, dummy.outputs));
  EXPECT_TRUE(dummy_stepper.step_called);
}
}  // namespace heph::conduit
