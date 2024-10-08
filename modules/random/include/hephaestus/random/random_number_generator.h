//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <random>

namespace heph::random {

namespace random_number_generator {
// TODO(@hruby) configure determinism via cmake
static constexpr bool IS_DETERMINISTIC = false;
}  // namespace random_number_generator

/// \brief Returns a random number generator (RNG) which is preconfigured to be deterministic or not.
/// 'is_deterministic' is exposed for unit testing, but shall in general not be set by the caller.
[[nodiscard]] auto createRNG(bool is_deterministic = random_number_generator::IS_DETERMINISTIC)
    -> std::mt19937_64;

/// \brief Returns a pair (but not a copy) of identical random number generators (RNG) which are preconfigured
/// to be deterministic, or not. This is useful for testing functions which require two random number
/// generators to be equal. 'is_deterministic' is exposed for unit testing, but shall in general not be set by
/// the caller.
[[nodiscard]] auto
createPairOfIdenticalRNGs(bool is_deterministic = random_number_generator::IS_DETERMINISTIC)
    -> std::pair<std::mt19937_64, std::mt19937_64>;

}  // namespace heph::random
