//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "eolo/serdes/protobuf/buffers.h"
#include "eolo/serdes/serdes.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace eolo::serdes::tests {

static constexpr int NUMBER = 42;

class MockProtoMessage {
public:
  // NOLINTNEXTLINE(modernize-use-trailing-return-type,bugprone-exception-escape)
  MOCK_METHOD(std::size_t, ByteSizeLong, (), (const));

  // NOLINTNEXTLINE(modernize-use-trailing-return-type,bugprone-exception-escape)
  MOCK_METHOD(bool, SerializeToArray, (void*, int), (const));

  // NOLINTNEXTLINE(modernize-use-trailing-return-type,bugprone-exception-escape)
  MOCK_METHOD(bool, ParseFromArray, (const void*, int), (const));
};

struct Data {
  int a = NUMBER;
};

}  // namespace eolo::serdes::tests

namespace eolo::serdes::protobuf {
template <>
struct ProtoAssociation<eolo::serdes::tests::Data> {
  using Type = eolo::serdes::tests::MockProtoMessage;
};
}  // namespace eolo::serdes::protobuf

namespace eolo::serdes::tests {

TEST(Protobuf, SerializerBuffers) {
  protobuf::SerializerBuffer buffer;
  MockProtoMessage proto;

  EXPECT_CALL(proto, ByteSizeLong).Times(1).WillOnce(Return(NUMBER));
  EXPECT_CALL(proto, SerializeToArray).Times(1);
  EXPECT_CALL(proto, ParseFromArray).Times(0);

  buffer.serialize(proto);

  auto data = std::move(buffer).exctractSerializedData();
  EXPECT_THAT(data, SizeIs(NUMBER));
}

TEST(Protobuf, DeserializerBuffers) {
  std::vector<std::byte> data{ NUMBER };
  protobuf::DeserializerBuffer buffer{ data };
  MockProtoMessage proto;

  EXPECT_CALL(proto, ByteSizeLong).Times(0);
  EXPECT_CALL(proto, SerializeToArray).Times(0);
  EXPECT_CALL(proto, ParseFromArray).Times(1).WillOnce(Return(true));

  auto res = buffer.deserialize(proto);

  EXPECT_TRUE(res);
}

void toProto(MockProtoMessage& proto, const Data& data) {
  EXPECT_CALL(proto, ByteSizeLong).Times(1).WillOnce(Return(data.a));
}

void fromProto(const MockProtoMessage& proto, Data& data) {
  (void)proto;
  data.a = NUMBER * 2;
}

TEST(Serialization, Protobuf) {
  Data data{};
  auto buffer = serialize(data);
  EXPECT_THAT(buffer, SizeIs(NUMBER));

  DefaultValue<bool>::Set(true);
  deserialize(buffer, data);
  EXPECT_EQ(data.a, NUMBER * 2);
}

}  // namespace eolo::serdes::tests
