//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstdint>

#include <gtest/gtest.h>

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::containers::tests {

TEST(BitFlag, EnumValuesPowerOfTwo) {
  enum class ValidEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u };
  static_assert(internal::checkEnumValuesArePowerOf2<ValidEnum>());

  enum class InValidEnum : uint8_t { A = 1u << 0u, B = 1u << 2u, C = 1u << 3u, D = 3u };
  static_assert(!internal::checkEnumValuesArePowerOf2<InValidEnum>());
}

TEST(BitFlag, AllEnumValuesMask) {
  enum class DenseEnum : uint8_t { A = 1u << 0u, B = 1u << 1u, C = 1u << 2u };
  static_assert(internal::allEnumValuesMask<DenseEnum>() == ((1u << 0u) | (1u << 1u) | (1u << 2u)));

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  enum class SparseEnum : uint8_t { A = 1u << 3u, B = 1u << 6u };

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  static_assert(internal::allEnumValuesMask<SparseEnum>() == ((1u << 3u) | (1u << 6u)));
}

enum class TestEnum : uint8_t { A = 1u << 0u, B = 1u << 1u, C = 1u << 2u, D = 1u << 3u };
TEST(BitFlag, Default) {
  const BitFlag<TestEnum> flag{ TestEnum::A };
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_FALSE(flag.has(TestEnum::B));
}

TEST(BitField, WithUnderlyingValue) {
  const BitFlag<TestEnum> flag{ (1u << 0u) | (1u << 2u) };
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_FALSE(flag.has(TestEnum::B));
  EXPECT_TRUE(flag.has(TestEnum::C));

  EXPECT_THROW_OR_DEATH(BitFlag<TestEnum>{ 1u << 4u }, InvalidParameterException, "contains invalid bits");
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

TEST(BitFlag, SetWithValueTrue) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::B, true);
  EXPECT_TRUE(flag.has(TestEnum::A));
  EXPECT_TRUE(flag.has(TestEnum::B));
  EXPECT_FALSE(flag.has(TestEnum::C));
}

TEST(BitFlag, SetWithValueFalse) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.set(TestEnum::B);

  flag.set(TestEnum::A, false);
  EXPECT_FALSE(flag.has(TestEnum::A));
  EXPECT_TRUE(flag.has(TestEnum::B));
  EXPECT_FALSE(flag.has(TestEnum::C));
}

TEST(BitFlag, Empty) {
  BitFlag<TestEnum> flag{ TestEnum::A };
  flag.unset(TestEnum::A);
  EXPECT_FALSE(flag.has(TestEnum::A));
}

TEST(BitFlag, UnderlyingValue) {
  static constexpr auto E = BitFlag{ TestEnum::A }.set(TestEnum::B).set(TestEnum::C);
  const auto value = E.getUnderlyingValue();
  EXPECT_EQ(static_cast<int>(value), 7);
}

}  // namespace heph::containers::tests
