//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types/uuid_v4.h"
#include "hephaestus/types_proto/bool.h"           // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/bounds.h"         // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/dummy_type.h"     // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/numeric_value.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/types_proto/uuid_v4.h"        // NOLINT(misc-include-cleaner)

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::types::tests {

struct TestTag {};
using IntegerBoundsT = Bounds<int32_t>;
using FloatingPointBoundsT = Bounds<float>;

template <class T>
class SerializationTests : public ::testing::Test {};
using SerializationImplementations =
    ::testing::Types<IntegerBoundsT, FloatingPointBoundsT, DummyType, UuidV4, bool,  //
                     int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float,
                     double  // NumericValue types
                     >;
TYPED_TEST_SUITE(SerializationTests, SerializationImplementations);

TYPED_TEST(SerializationTests, TestEmptySerialization) {
  const TypeParam value{};
  TypeParam value_des{};

  auto buff = serdes::serialize(value);
  serdes::deserialize(buff, value_des);

  EXPECT_EQ(value, value_des);
}

TYPED_TEST(SerializationTests, TestSerialization) {
  auto mt = random::createRNG();
  const auto value = random::random<TypeParam>(mt);
  auto buff = serdes::serialize(value);

  {
    // Ensure that randomized value is not equal to the default constructed value
    const TypeParam value_des{};
    if constexpr (!std::is_same_v<TypeParam, bool> && !std::is_same_v<TypeParam, int8_t> &&
                  !std::is_same_v<TypeParam, uint8_t>) {  // sample space of bool and small integers is too
                                                          // small
      EXPECT_NE(value, value_des);
    }
  }

  {
    // Deserialize into a empty object
    TypeParam value_des{};
    heph::serdes::deserialize(buff, value_des);
    EXPECT_EQ(value, value_des);
  }

  {
    // Deserialize into a non-empty object
    auto value_des = random::random<TypeParam>(mt);
    heph::serdes::deserialize(buff, value_des);
    EXPECT_EQ(value, value_des);
  }
}

}  // namespace heph::types::tests
