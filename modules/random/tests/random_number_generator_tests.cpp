//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <type_traits>

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::random {

// Test that createRNG returns a std::mt19937_64 object
TEST(RNGTests, ReturnsMersenneTwisterEngine) {
  [[maybe_unused]] auto mt = createRNG();
  static_assert(std::is_same_v<decltype(mt), std::mt19937_64>, "createRNG must return std::mt19937_64");
}

// Test that if IS_DETERMINISTIC is true, two generated random number generators are equal
TEST(RNGTests, GeneratorsAreDeterministic) {
  {
    const bool is_deterministic = true;
    auto mt1 = createRNG(is_deterministic);
    auto mt2 = createRNG(is_deterministic);
    ASSERT_EQ(mt1(), mt2()) << "Random number generators must produce the same sequence when deterministic";
    mt1();
    ASSERT_NE(mt1(), mt2()) << "Random number generators must produce different numbers after advancing";
  }

  {
    const bool is_deterministic = false;
    auto mt1 = createRNG(is_deterministic);
    auto mt2 = createRNG(is_deterministic);
    ASSERT_NE(mt1(), mt2()) << "Random number generators must not produce the same sequence when non "
                               "deterministic";
  }
}

// Test that createPairOfIdenticalRNGs returns a std::mt19937_64 object
TEST(RNGTests, PairGeneratorReturnsMersenneTwisterEngines) {
  auto [mt1, mt2] = createPairOfIdenticalRNGs();
  static_assert(std::is_same_v<decltype(mt1), std::mt19937_64>,
                "createPairOfIdenticalRNGs must return std::mt19937_64");
  static_assert(std::is_same_v<decltype(mt2), std::mt19937_64>,
                "createPairOfIdenticalRNGs must return std::mt19937_64");
}

// Test that createPairOfIdenticalRNGs returns a pair of identical engines
TEST(RNGTests, PairGeneratorsAreIdentical) {
  auto [mt1, mt2] = createPairOfIdenticalRNGs();
  ASSERT_EQ(mt1(), mt2()) << "The pair of random number generators must be identical";
  mt1();
  ASSERT_NE(mt1(), mt2()) << "The pair of random number generators must produce different numbers after "
                             "advancing";
}

// Test that the engines from createPairOfIdenticalRNGs remain identical after some uses
TEST(RNGTests, PairGeneratorsStayIdenticalAfterUse) {
  auto [mt1, mt2] = createPairOfIdenticalRNGs();

  // Generate some random numbers to advance the state of the engines
  static constexpr auto RNG_ITERATION_COUNT = 10;
  for (std::size_t count = 0; count < RNG_ITERATION_COUNT; ++count) {
    mt1();
    mt2();
  }

  ASSERT_EQ(mt1(), mt2()) << "The pair of random number generators must stay identical even after "
                             "generating some numbers";
}

}  // namespace heph::random
