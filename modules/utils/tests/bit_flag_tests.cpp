//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <gtest/gtest.h>

#include "hephaestus/utils/bit_flag.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {

TEST(BitFlag, EnumValuesPowerOfTwo) {
  enum class ValidEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u };
  static_assert(checkEnumValuesArePowerOf2<ValidEnum>());

  enum class InValidEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u, D = 3u };
  static_assert(!checkEnumValuesArePowerOf2<InValidEnum>());
}

enum class TestEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u, D = 1u << 4u };
TEST(BitFlag, Default) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_FALSE(flag.has(TestEnum::B));
}

TEST(BitFlag, Reset) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.reset(TestEnum::B);
  EXPECT_FALSE(flag.has(TestEnum::A));
  EXPECT_TRUE(flag.has(TestEnum::B));
}

TEST(BitFlag, Set) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::B);
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_TRUE(flag.has(TestEnum::B));
}

TEST(BitFlag, SetMultiple) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  static constexpr auto E = BitFlag<TestEnum>{ TestEnum::C }.set(TestEnum::D);
  flag.set(E);
  EXPECT_TRUE(flag.has(TestEnum::C));
  EXPECT_TRUE(flag.has(TestEnum::D));
}

TEST(BitFlag, HasAny) {
  static constexpr auto E = BitFlag<TestEnum>{ TestEnum::C }.set(TestEnum::D);

  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::C).set(TestEnum::D);
  EXPECT_TRUE(flag.has(E));
  EXPECT_TRUE(flag.hasAny(E));

  flag.unset(TestEnum::C);
  EXPECT_FALSE(flag.has(E));
  EXPECT_TRUE(flag.hasAny(E));
}

TEST(BitFlag, HasExactly) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::B).set(TestEnum::C);
  EXPECT_FALSE(flag.hasExactly(TestEnum::A));

  static constexpr auto E = BitFlag<TestEnum>{ TestEnum::A }.set(TestEnum::B).set(TestEnum::C);
  EXPECT_TRUE(flag.hasExactly(E));
}

TEST(BitFlag, Unset) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.unset(TestEnum::A);
  EXPECT_FALSE(flag.has(TestEnum::A));
}

TEST(BitFlag, UnsetMultiple) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  static constexpr auto E = BitFlag<TestEnum>{ TestEnum::C }.set(TestEnum::D);
  flag.set(E);
  flag.unset(E);
  EXPECT_FALSE(flag.has(TestEnum::C));
  EXPECT_FALSE(flag.has(TestEnum::D));
  EXPECT_FALSE(flag.has(E));
}

}  // namespace heph::utils::tests
