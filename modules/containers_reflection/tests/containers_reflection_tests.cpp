//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
// #include <rfl.hpp>  // NOLINT(misc-include-cleaner)

#include <rfl/to_view.hpp>
#include <rfl/yaml/read.hpp>
#include <rfl/yaml/write.hpp>

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/containers_reflection/bit_flag.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/containers_reflection/chrono.h"    // NOLINT(misc-include-cleaner)

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::containers::tests {

TEST(Reflection, BitFlag) {
  enum class Enum : uint8_t { A = 1u << 3u, B = 1u << 5u };
  struct TestStruct {
    BitFlag<Enum> flag{ Enum::B };
  };
  {
    const TestStruct test;
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

TEST(Reflection, TestYAMLRoundtripWithChronoDuration) {
  struct TestStruct {
    std::string a = "test_value";
    std::chrono::duration<double> b = std::chrono::minutes{ 42 };
    std::chrono::milliseconds c = std::chrono::milliseconds{ 42 };
  };
  const TestStruct x{};
  const auto yaml = rfl::yaml::write(x);

  const auto parsed = rfl::yaml::read<TestStruct>(yaml);
  EXPECT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->a, x.a);
  EXPECT_EQ(parsed->b, x.b);
  EXPECT_EQ(parsed->c, x.c);
}

TEST(Reflection, TestYAMLWithChronoDurationError) {
  {
    const std::string yaml = R"(100s)";
    const auto parsed = rfl::yaml::read<std::chrono::seconds>(yaml);
    EXPECT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, std::chrono::seconds(100));
  }

  {
    const std::string yaml = R"(100)";
    const auto parsed = rfl::yaml::read<std::chrono::seconds>(yaml);
    EXPECT_FALSE(parsed.has_value());
  }

  {
    const std::string yaml = R"(asbms)";
    const auto parsed = rfl::yaml::read<std::chrono::seconds>(yaml);
    EXPECT_FALSE(parsed.has_value());
  }

  {
    const std::string yaml = R"(100ms)";
    const auto parsed = rfl::yaml::read<std::chrono::milliseconds>(yaml);
    EXPECT_FALSE(parsed.has_value());
  }
}

}  // namespace heph::containers::tests
