//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>

#include <gtest/gtest.h>

#include "hephaestus/testing/random_container.h"
#include "hephaestus/testing/random_generator.h"
#include "hephaestus/testing/random_type.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::testing {

/// Compare the results of a randomly generated type multiple times to ensure that it is not equal by chance.
template <class T>
[[nodiscard]] auto compareRandomEqualMultipleTimes(std::function<T(std::mt19937_64&)> gen,
                                                   std::mt19937_64& mt) -> bool {
  static constexpr size_t MAX_COMPARISON_COUNT = 10;

  for (size_t tries = 0; tries < MAX_COMPARISON_COUNT; ++tries) {
    T first_result = gen(mt);
    T second_result = gen(mt);

    if (first_result != second_result) {
      return false;
    }
  }

  return true;
}

enum class TestEnum : int8_t { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P };

struct TestStruct {
  int a;
  double b;
  std::string c;
  std::vector<int> d;

  [[nodiscard]] auto operator==(const TestStruct&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> TestStruct {
    return { randomT<int>(mt), randomT<double>(mt), randomT<std::string>(mt), randomT<std::vector<int>>(mt) };
  }
};

template <class T>
class RandomTypeTests : public ::testing::Test {};
using RandomTypeImplementations = ::testing::Types<
    /* Integer types */ int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t,
    /* Floating point types */ float, double, long double,
    /* Enum type */ TestEnum,
    /* String types */ std::string,
    /* Timestamp type */ std::chrono::time_point<std::chrono::system_clock>,
    /* Container types */ std::vector<int>, std::vector<double>, std::vector<std::string>,
    /* Custom types */ TestStruct>;
TYPED_TEST_SUITE(RandomTypeTests, RandomTypeImplementations);

TYPED_TEST(RandomTypeTests, DeterminismTest) {
  auto [mt, mt_copy] = createPairOfIdenticalRNGs();
  EXPECT_EQ(randomT<TypeParam>(mt), randomT<TypeParam>(mt_copy));
}

TYPED_TEST(RandomTypeTests, RandomnessTest) {
  auto mt = createRNG();
  auto gen = [](std::mt19937_64& gen) -> TypeParam { return randomT<TypeParam>(gen); };
  EXPECT_FALSE(compareRandomEqualMultipleTimes<TypeParam>(gen, mt));
}

// Note: If the size of the container is not specified, the size is randomly generated. No need to test this
// case, as it is already included in testing for randomness. Repeadedly creating an empty container would
// fail the RandomnessTest.
TYPED_TEST(RandomTypeTests, ContainerSizeTest) {
  if constexpr (IsRandomGeneratableVectorT<TypeParam>) {
    auto mt = createRNG();

    static constexpr size_t SIZE_ZERO = 0;
    auto vec_size_zero = randomT<TypeParam>(mt, SIZE_ZERO);
    EXPECT_EQ(vec_size_zero.size(), SIZE_ZERO);

    static constexpr size_t SIZE_SEVEN = 7;
    auto vec_size_seven = randomT<TypeParam>(mt, SIZE_SEVEN);
    EXPECT_EQ(vec_size_seven.size(), SIZE_SEVEN);
  }
}

}  // namespace heph::testing
