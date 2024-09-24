//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <array>          // NOLINT(misc-include-cleaner)
#include <chrono>         // NOLINT(misc-include-cleaner)
#include <cstdint>        // NOLINT(misc-include-cleaner)
#include <map>            // NOLINT(misc-include-cleaner)
#include <string>         // NOLINT(misc-include-cleaner)
#include <unordered_map>  // NOLINT(misc-include-cleaner)
#include <utility>        // NOLINT(misc-include-cleaner)
#include <vector>         // NOLINT(misc-include-cleaner)

#include <gtest/gtest.h>

#include "hephaestus/utils/concepts.h"  // NOLINT(misc-include-cleaner)

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {

TEST(ConceptTests, ScalarType) {
  EXPECT_TRUE((ScalarType<int>));
  EXPECT_TRUE((ScalarType<int8_t>));
  EXPECT_TRUE((ScalarType<int16_t>));
  EXPECT_TRUE((ScalarType<int32_t>));
  EXPECT_TRUE((ScalarType<int64_t>));
  EXPECT_TRUE((ScalarType<uint8_t>));
  EXPECT_TRUE((ScalarType<uint16_t>));
  EXPECT_TRUE((ScalarType<uint32_t>));
  EXPECT_TRUE((ScalarType<uint64_t>));

  EXPECT_TRUE((ScalarType<float>));
  EXPECT_TRUE((ScalarType<double>));

  EXPECT_TRUE((ScalarType<char>));
  EXPECT_TRUE((ScalarType<unsigned char>));
  EXPECT_TRUE((ScalarType<bool>));

  EXPECT_FALSE((ScalarType<std::vector<int>>));
}

TEST(ConceptTests, EnumType) {
  enum class TestEnum : uint8_t { A, B, C };
  EXPECT_TRUE((EnumType<TestEnum>));
  EXPECT_FALSE((EnumType<int>));
  EXPECT_FALSE((EnumType<std::vector<int>>));
}

TEST(ConceptTests, ArrayType) {
  EXPECT_TRUE((ArrayType<std::array<int, 5>>));
  EXPECT_FALSE((ArrayType<std::vector<int>>));
  EXPECT_FALSE((ArrayType<int[5]>));  // NOLINT(cppcoreguidelines-avoid-c-arrays)
}

TEST(ConceptTests, VectorType) {
  EXPECT_TRUE((VectorType<std::vector<int>>));
  EXPECT_FALSE((VectorType<std::unordered_map<int, std::string>>));
  EXPECT_FALSE((VectorType<std::array<int, 5>>));
  EXPECT_FALSE((VectorType<int>));
}

TEST(ConceptTests, UnorderedMapType) {
  EXPECT_TRUE((UnorderedMapType<std::unordered_map<int, std::string>>));
  EXPECT_FALSE((UnorderedMapType<std::map<int, std::string>>));
  EXPECT_FALSE((UnorderedMapType<std::vector<std::pair<int, std::string>>>));
}

TEST(ConceptTests, ChronoSystemClockType) {
  EXPECT_TRUE((ChronoSystemClockType<std::chrono::system_clock>));
  EXPECT_FALSE((ChronoSystemClockType<std::chrono::steady_clock>));
  EXPECT_FALSE((ChronoSystemClockType<int>));
}

TEST(ConceptTests, ChronoSteadyClockType) {
  EXPECT_TRUE((ChronoSteadyClockType<std::chrono::steady_clock>));
  EXPECT_FALSE((ChronoSteadyClockType<std::chrono::system_clock>));
  EXPECT_FALSE((ChronoSteadyClockType<int>));
}

}  // namespace heph::utils::tests
