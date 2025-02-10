//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hephaestus/utils/stack_trace.h>
#include <nlohmann/detail/macro_scope.hpp>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/serdes/serdes.h"
#include "test_proto_conversion.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::serdes::tests {
namespace {

TEST(TypeInfo, Generate) {
  auto type_info = getSerializedTypeInfo<User>();
  EXPECT_EQ(type_info.name, "heph.serdes.tests.proto.User");
  EXPECT_EQ(type_info.original_type, "heph::serdes::tests::User");
  EXPECT_EQ(type_info.serialization, TypeInfo::Serialization::PROTOBUF);
  EXPECT_FALSE(type_info.schema.empty());
}

[[nodiscard]] auto randomTypeInfo(std::mt19937_64& mt) -> TypeInfo {
  return {
    .name = random::random<std::string>(mt, std::nullopt, false, true),
    .schema = random::random<std::vector<std::byte>>(mt, std::nullopt, false),
    .serialization = random::random<TypeInfo::Serialization>(mt),
    .original_type = random::random<std::string>(mt, std::nullopt, false, true),
  };
}

TEST(TypeInfo, ToFromJson) {
  utils::StackTrace trace;
  auto mt = random::createRNG();

  const auto type_info = randomTypeInfo(mt);

  const auto json = type_info.toJson();
  const auto new_type_info = TypeInfo::fromJson(json);

  EXPECT_EQ(type_info, new_type_info);
}

TEST(ServiceTypeInfo, ToFromJson) {
  utils::StackTrace trace;
  auto mt = random::createRNG();

  const auto service_type_info = ServiceTypeInfo{
    .request = randomTypeInfo(mt),
    .reply = randomTypeInfo(mt),
  };

  const auto json = service_type_info.toJson();
  const auto new_service_type_info = ServiceTypeInfo::fromJson(json);

  EXPECT_EQ(service_type_info, new_service_type_info);
}
}  // namespace
}  // namespace heph::serdes::tests
