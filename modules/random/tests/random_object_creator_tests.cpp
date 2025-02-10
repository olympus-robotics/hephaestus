//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/utils/concepts.h"
#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::random {
namespace {
/// Compare the results of a randomly generated type multiple times to ensure that it is not equal by chance.
template <class T>
[[nodiscard]] auto compareRandomEqualMultipleTimes(const std::function<T(std::mt19937_64&)>& gen,
                                                   std::mt19937_64& mt) -> bool {
  static constexpr std::size_t MAX_COMPARISON_COUNT = 100;

  for (std::size_t tries = 0; tries < MAX_COMPARISON_COUNT; ++tries) {
    const T first_result = gen(mt);
    const T second_result = gen(mt);

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
  std::byte e;

  [[nodiscard]] auto operator==(const TestStruct&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> TestStruct {
    return { .a = random::random<int>(mt),
             .b = random::random<double>(mt),
             .c = random::random<std::string>(mt),
             .d = random::random<std::vector<int>>(mt),
             .e = random::random<std::byte>(mt) };
  }
};

template <class T>
class RandomTypeTests : public ::testing::Test {};
using RandomTypeImplementations = ::testing::Types<
    /* Boolean type */ bool,
    /* Integer types */ int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t,
    /* Floating point types */ float, double, long double,
    /* Enum type */ TestEnum,
    /* Optional type */ std::optional<TestStruct>,
    /* Timestamp type */ std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>,
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>,
    std::chrono::time_point<std::chrono::steady_clock>,
    /* Container types */ std::string, std::vector<int>, std::vector<double>, std::vector<std::vector<int>>,
    std::vector<std::vector<double>>,
    /* Array types*/ std::array<int, 4>, std::array<double, 4>, std::vector<std::array<int, 4>>,
    std::vector<std::array<double, 4>>,
    /* Custom types */ TestStruct>;
TYPED_TEST_SUITE(RandomTypeTests, RandomTypeImplementations);

TYPED_TEST(RandomTypeTests, DeterminismTest) {
  auto [mt, mt_copy] = createPairOfIdenticalRNGs();
  EXPECT_EQ(random::random<TypeParam>(mt), random::random<TypeParam>(mt_copy));
}

TYPED_TEST(RandomTypeTests, RandomnessTest) {
  auto mt = createRNG();
  auto gen_fn = [](std::mt19937_64& gen) -> TypeParam { return random::random<TypeParam>(gen); };
  EXPECT_FALSE(compareRandomEqualMultipleTimes<TypeParam>(gen_fn, mt));
}

TYPED_TEST(RandomTypeTests, LimitsTest) {
  if constexpr (NonBooleanIntegralType<TypeParam> || std::floating_point<TypeParam>) {
    constexpr auto LIM_MIN = std::is_signed_v<TypeParam> ? -42 : 0;
    constexpr auto LIM_MAX = 42;
    auto mt = createRNG();
    auto limits = random::Limits<TypeParam>{ .min = LIM_MIN, .max = LIM_MAX };
    auto val = random::random<TypeParam>(mt, limits);
    EXPECT_GE(val, limits.min);
    EXPECT_LE(val, limits.max);
  }
}

// Note: If the size of the container is not specified, the size is randomly generated. No need to test this
// case, as it is already included in testing for randomness. Repeadedly creating an empty container would
// fail the RandomnessTest.
TYPED_TEST(RandomTypeTests, ContainerSizeTest) {
  auto mt = createRNG();
  if constexpr (RandomCreatableVector<TypeParam> || RandomCreatableVectorOfVectors<TypeParam> ||
                RandomCreatableVectorOfArrays<TypeParam> || StringType<TypeParam>) {
    static constexpr std::size_t SIZE_ZERO = 0;
    static constexpr bool ALLOW_EMPTY_CONTAINER = true;
    static constexpr bool DISALLOW_EMPTY_CONTAINER = false;
    auto vec_size_zero = random::random<TypeParam>(mt, SIZE_ZERO, ALLOW_EMPTY_CONTAINER);
    EXPECT_EQ(vec_size_zero.size(), SIZE_ZERO);
    EXPECT_THROW_OR_DEATH(auto _ = random::random<TypeParam>(mt, SIZE_ZERO, DISALLOW_EMPTY_CONTAINER),
                          InvalidParameterException, "");

    static constexpr size_t SIZE_SEVEN = 7;
    auto vec_size_seven = random::random<TypeParam>(mt, SIZE_SEVEN);
    EXPECT_EQ(vec_size_seven.size(), SIZE_SEVEN);

    auto random_vec_non_empty = random::random<TypeParam>(mt, {}, DISALLOW_EMPTY_CONTAINER);
    EXPECT_TRUE(vec_size_seven.size() > 0);
  }
}

}  // namespace
}  // namespace heph::random
