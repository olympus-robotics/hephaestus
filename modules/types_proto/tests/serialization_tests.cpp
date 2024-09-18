//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/types_proto/bounds.h"
#include "hephaestus/types_proto/dummy_type.h"
#include "hephaestus/types_proto/numeric_value.h"

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::types::tests {

struct TestTag {};
using IntegerBoundsT = Bounds<int32_t>;
using FloatingPointBoundsT = Bounds<float>;

template <class T>
class SerializationTests : public ::testing::Test {};
using SerializationImplementations =
    ::testing::Types<IntegerBoundsT, FloatingPointBoundsT, DummyType,           //
                     int32_t, int64_t, uint32_t, uint64_t, float, double, bool  // NumericValue types
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

  TypeParam value_des{};
  EXPECT_NE(value, value_des);

  auto buff = serdes::serialize(value);
  serdes::deserialize(buff, value_des);

  EXPECT_EQ(value, value_des);
}

}  // namespace heph::types::tests
