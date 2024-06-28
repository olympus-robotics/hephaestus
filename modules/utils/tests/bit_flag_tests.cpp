//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstdint>

#include <gtest/gtest.h>

#include "hephaestus/utils/bit_flag.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {

TEST(BitFlag, EnumValuesPowerOfTwo) {
  enum class ValidEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u };
  static_assert(internal::checkEnumValuesArePowerOf2<ValidEnum>());

  enum class InValidEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u, D = 3u };
  static_assert(!internal::checkEnumValuesArePowerOf2<InValidEnum>());
}

enum class TestEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u, D = 1u << 4u };
TEST(BitFlag, Default) {
  const BitFlag<TestEnum> flag{ TestEnum::A };
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_FALSE(flag.has(TestEnum::B));
}

TEST(BitFlag, Reset) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.reset();
  EXPECT_FALSE(flag.has(TestEnum::A));
  constexpr auto ALL = BitFlag{ TestEnum::A }.set(TestEnum::B).set(TestEnum::C).set(TestEnum::D);
  EXPECT_FALSE(flag.hasAnyOf(ALL));
}

TEST(BitFlag, Set) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::B);
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_TRUE(flag.has(TestEnum::B));
}

TEST(BitFlag, SetMultiple) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  static constexpr auto E = BitFlag{ TestEnum::C }.set(TestEnum::D);
  flag.set(E);
  EXPECT_TRUE(flag.has(TestEnum::C));
  EXPECT_TRUE(flag.has(TestEnum::D));
}

TEST(BitFlag, hasAnyOf) {
  static constexpr auto E = BitFlag{ TestEnum::C }.set(TestEnum::D);

  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::C).set(TestEnum::D);
  EXPECT_TRUE(flag.has(E));
  EXPECT_TRUE(flag.hasAnyOf(E));

  flag.unset(TestEnum::C);
  EXPECT_FALSE(flag.has(E));
  EXPECT_TRUE(flag.hasAnyOf(E));
}

TEST(BitFlag, HasExactly) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::B).set(TestEnum::C);
  EXPECT_FALSE(flag.hasExactly(TestEnum::A));

  static constexpr auto E = BitFlag{ TestEnum::A }.set(TestEnum::B).set(TestEnum::C);
  EXPECT_TRUE(flag.hasExactly(E));
}

TEST(BitFlag, Unset) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.unset(TestEnum::A);
  EXPECT_FALSE(flag.has(TestEnum::A));
}

TEST(BitFlag, UnsetMultiple) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  static constexpr auto E = BitFlag{ TestEnum::C }.set(TestEnum::D);
  flag.set(E);
  flag.unset(E);
  EXPECT_FALSE(flag.has(TestEnum::C));
  EXPECT_FALSE(flag.has(TestEnum::D));
  EXPECT_FALSE(flag.has(E));
}

}  // namespace heph::utils::tests
