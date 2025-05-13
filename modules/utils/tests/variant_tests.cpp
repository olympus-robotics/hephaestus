//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================
#include <string>
#include <variant>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/variant.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::tests {
TEST(Variant, VariantOverloads) {
  using Variant = std::variant<std::string, int, float>;

  auto overloads = Overloads{
    [](const std::string& value) { return fmt::format("Holds string \"{}\"", value); },
    [](const int value) { return fmt::format("Holds int {}", value); },
    [](const float value) { return fmt::format("Holds float {}", value); },
  };

  EXPECT_EQ(std::visit(overloads, Variant("foo")), "Holds string \"foo\"");
  EXPECT_EQ(std::visit(overloads, Variant(2)), "Holds int 2");
  EXPECT_EQ(std::visit(overloads, Variant(3.0f)), "Holds float 3");
}

}  // namespace heph::tests
