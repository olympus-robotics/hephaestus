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

#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/stepper.h"

namespace heph::conduit {
struct Dummy {
  int value{ -1 };
  void initialize(const std::string& /*prefix*/, void* /*unused*/, int new_value) {
    value = new_value;
  }
};
struct DummyNodeDescription : NodeDescriptionDefaults<DummyNodeDescription> {
  static constexpr std::string_view NAME{ "dummy" };
  struct Inputs {
    int value{ 0 };
  };
  struct Outputs {
    int value{ 1 };
  };
  struct Children {
    Dummy value;
  };
  struct ChildrenConfig {
    int value{ 3 };
  };
};

struct DummyStepper : StepperDefaults<DummyNodeDescription> {
  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    return {
      .value = 0,
    };
  }
};

TEST(Node, Basic) {
  DummyStepper dummy_stepper;
  Node<DummyNodeDescription> dummy;
  dummy.initialize("", nullptr, dummy_stepper);
  EXPECT_EQ(dummy->children.value.value, 0);
}

struct DummyParentNodeDescription : NodeDescriptionDefaults<DummyParentNodeDescription> {
  static constexpr std::string_view NAME{ "parent" };
  struct Children {
    Node<DummyNodeDescription> child;
  };
  struct ChildrenConfig {
    DummyNodeDescription::StepperT dummy_stepper;
  };
};

struct DummyParentStepper : StepperDefaults<DummyParentNodeDescription> {
  DummyStepper dummy_stepper;
  [[nodiscard]] auto childrenConfig() const -> ChildrenConfigT {
    return { dummy_stepper };
  }
};

TEST(Node, Hierarchy) {
  DummyParentStepper dummy_stepper{};
  Node<DummyParentNodeDescription> dummy;
  dummy.initialize("", nullptr, dummy_stepper);
  EXPECT_EQ(dummy->children.child->children.value.value, 0);
}
}  // namespace heph::conduit
