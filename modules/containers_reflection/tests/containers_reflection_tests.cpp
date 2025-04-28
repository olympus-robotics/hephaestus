//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>

#include <gtest/gtest.h>
// #include <rfl.hpp>  // NOLINT(misc-include-cleaner)
#include <rfl/to_view.hpp>
// #include <rfl/yaml.hpp>  // NOLINT(misc-include-cleaner)
#include <rfl/yaml/write.hpp>

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/containers_reflection/bit_flag.h"  // NOLINT(misc-include-cleaner)

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::containers::tests {

TEST(Reflection, BitFlag) {
  enum class Enum : uint8_t { A = 1u << 3u, B = 1u << 5u };
  struct Test {
    BitFlag<Enum> flag{ Enum::B };
  };
  {
    const Test test;
    const auto view = rfl::to_view(test);
    EXPECT_EQ(view.size(), 1);

    const auto yaml = rfl::yaml::write(test);
    EXPECT_EQ(yaml, "flag: 32");
  }
  {
    const BitFlag<Enum> flag{ Enum::B };
    const auto yaml = rfl::yaml::write(flag);
    EXPECT_EQ(yaml, "32");
  }
}

}  // namespace heph::containers::tests
