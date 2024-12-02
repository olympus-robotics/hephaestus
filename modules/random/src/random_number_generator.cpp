//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/random/random_number_generator.h"

#include <cstdint>
#include <random>
#include <utility>

#include <fmt/base.h>

namespace heph::random {

namespace {

[[nodiscard]] auto getSeed(bool is_deterministic) -> uint64_t {
  if (is_deterministic) {
    static constexpr uint64_t FIXED_SEED = 42;
    return FIXED_SEED;
  }

  const auto seed = std::random_device{}();
  fmt::println("test_helper RNG seed: {}", seed);
  return seed;
}

}  // namespace

auto createRNG(bool is_deterministic) -> std::mt19937_64 {
  const auto seed = getSeed(is_deterministic);
  return std::mt19937_64{ seed };
}

auto createPairOfIdenticalRNGs(bool is_deterministic) -> std::pair<std::mt19937_64, std::mt19937_64> {
  const auto seed = getSeed(is_deterministic);
  return { std::mt19937_64{ seed }, std::mt19937_64{ seed } };
}

}  // namespace heph::random
