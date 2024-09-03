//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::types::tests {

struct TestTag {};

template <class T>
class SerializationTests : public ::testing::Test {};
using SerializationImplementations = ::testing::Types<DummyType>;
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
  const auto value = TypeParam::random(mt);

  TypeParam value_des{};
  EXPECT_NE(value, value_des);

  auto buff = serdes::serialize(value);
  serdes::deserialize(buff, value_des);

  EXPECT_EQ(value, value_des);
}

}  // namespace heph::types::tests
