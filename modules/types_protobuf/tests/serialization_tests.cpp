//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/random/random_generator.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/types_protobuf/dummy_type.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

struct TestTag {};

template <class T>
class SerializationTests : public ::testing::Test {};
using SerializationImplementations = ::testing::Types<DummyType>;
TYPED_TEST_SUITE(SerializationTests, SerializationImplementations);

TYPED_TEST(SerializationTests, TestEmptySerialization) {
  const TypeParam value{};
  TypeParam value_des{};

  auto buff = heph::serdes::serialize(value);
  heph::serdes::deserialize(buff, value_des);

  EXPECT_EQ(value, value_des);
}

TYPED_TEST(SerializationTests, TestSerialization) {
  auto mt = heph::random::createRNG();
  const auto value = TypeParam::random(mt);

  TypeParam value_des{};
  EXPECT_NE(value, value_des);

  auto buff = heph::serdes::serialize(value);
  heph::serdes::deserialize(buff, value_des);

  EXPECT_EQ(value, value_des);
}

}  // namespace heph::types::tests
