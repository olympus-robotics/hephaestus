#include <foxglove/websocket/common.hpp>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>
#include <hephaestus/websocket_bridge/protobuf_utils.h>

namespace heph::ws_bridge::tests {

class ProtobufUtilsTest : public ::testing::Test {
protected:
  ProtobufSchemaDatabase schema_db;
  RandomGenerators generators;

  void SetUp() override {
    // Initialize schema_db and generators if needed
  }

  void TearDown() override {
    // Clean up if needed
  }
};

TEST_F(ProtobufUtilsTest, LoadSchemaValid) {
  google::protobuf::FileDescriptorSet descriptor_set;
  auto file_descriptor_proto = descriptor_set.add_file();
  file_descriptor_proto->set_name("test.proto");
  file_descriptor_proto->set_package("test");

  std::vector<std::byte> schema_bytes(descriptor_set.ByteSizeLong());
  descriptor_set.SerializeToArray(schema_bytes.data(), static_cast<int>(schema_bytes.size()));

  EXPECT_TRUE(loadSchema(schema_bytes, schema_db.proto_db.get()));
}

TEST_F(ProtobufUtilsTest, LoadSchemaInvalid) {
  std::vector<std::byte> schema_bytes = { std::byte{ 0x00 }, std::byte{ 0x01 } };
  EXPECT_FALSE(loadSchema(schema_bytes, schema_db.proto_db.get()));
}

TEST_F(ProtobufUtilsTest, SaveAndRetrieveSchemaFromDatabase) {
  foxglove::Service service_definition;
  service_definition.id = 42;
  service_definition.name = "Poser";
  service_definition.type = "";

  service_definition.request = foxglove::ServiceRequestDefinition{
    .encoding = "protobuf",
    .schemaName = "heph.examples.types.proto.Pose",
    .schemaEncoding = "protobuf",
    .schema =
        "Cv4CCipoZXBoYWVzdHVzL2V4YW1wbGVzL3R5cGVzL3Byb3RvL3Bvc2UucHJvdG8SGWhlcGguZXhhbXBsZXMudHlwZXMucHJvdG8a"
        "LmhlcGhhZXN0dXMvZXhhbXBsZXMvdHlwZXMvcHJvdG8vZ2VvbWV0cnkucHJvdG8isAEKBFBvc2USNQoIcG9zaXRpb24YASABKAsy"
        "Iy5oZXBoLmV4YW1wbGVzLnR5cGVzLnByb3RvLlZlY3RvcjNkEjsKC29yaWVudGF0aW9uGAIgASgLMiYuaGVwaC5leGFtcGxlcy50"
        "eXBlcy5wcm90by5RdWF0ZXJuaW9uZBISCgpjb25maWRlbmNlGAMgASgBEg0KBWZyYW1lGAQgASgJEhEKCXRpbWVzdGFtcBgFIAEo"
        "ASJKCgpGcmFtZWRQb3NlEg0KBWZyYW1lGAEgASgJEi0KBHBvc2UYAiABKAsyHy5oZXBoLmV4YW1wbGVzLnR5cGVzLnByb3RvLlBv"
        "c2ViBnByb3RvMwrjAgouaGVwaGFlc3R1cy9leGFtcGxlcy90eXBlcy9wcm90by9nZW9tZXRyeS5wcm90bxIZaGVwaC5leGFtcGxl"
        "cy50eXBlcy5wcm90byIrCghWZWN0b3IzZBIJCgF4GAEgASgBEgkKAXkYAiABKAESCQoBehgDIAEoASI5CgtRdWF0ZXJuaW9uZBIJ"
        "CgF4GAEgASgBEgkKAXkYAiABKAESCQoBehgDIAEoARIJCgF3GAQgASgBIjQKCE1hdHJpeFhkEgwKBHJvd3MYASABKA0SDAoEY29s"
        "cxgCIAEoDRIMCgRkYXRhGAMgAygBIjQKCE1hdHJpeFhmEgwKBHJvd3MYASABKA0SDAoEY29scxgCIAEoDRIMCgRkYXRhGAMgAygC"
        "IhgKCFZlY3RvclhmEgwKBGRhdGEYASADKAIiIAoIVmVjdG9yMmYSCQoBeBgBIAEoAhIJCgF5GAIgASgCYgZwcm90bzM=",
  };
  service_definition.response = foxglove::ServiceResponseDefinition{
    .encoding = "protobuf",
    .schemaName = "heph.examples.types.proto.Pose",
    .schemaEncoding = "protobuf",
    .schema =
        "Cv4CCipoZXBoYWVzdHVzL2V4YW1wbGVzL3R5cGVzL3Byb3RvL3Bvc2UucHJvdG8SGWhlcGguZXhhbXBsZXMudHlwZXMucHJvdG8a"
        "LmhlcGhhZXN0dXMvZXhhbXBsZXMvdHlwZXMvcHJvdG8vZ2VvbWV0cnkucHJvdG8isAEKBFBvc2USNQoIcG9zaXRpb24YASABKAsy"
        "Iy5oZXBoLmV4YW1wbGVzLnR5cGVzLnByb3RvLlZlY3RvcjNkEjsKC29yaWVudGF0aW9uGAIgASgLMiYuaGVwaC5leGFtcGxlcy50"
        "eXBlcy5wcm90by5RdWF0ZXJuaW9uZBISCgpjb25maWRlbmNlGAMgASgBEg0KBWZyYW1lGAQgASgJEhEKCXRpbWVzdGFtcBgFIAEo"
        "ASJKCgpGcmFtZWRQb3NlEg0KBWZyYW1lGAEgASgJEi0KBHBvc2UYAiABKAsyHy5oZXBoLmV4YW1wbGVzLnR5cGVzLnByb3RvLlBv"
        "c2ViBnByb3RvMwrjAgouaGVwaGFlc3R1cy9leGFtcGxlcy90eXBlcy9wcm90by9nZW9tZXRyeS5wcm90bxIZaGVwaC5leGFtcGxl"
        "cy50eXBlcy5wcm90byIrCghWZWN0b3IzZBIJCgF4GAEgASgBEgkKAXkYAiABKAESCQoBehgDIAEoASI5CgtRdWF0ZXJuaW9uZBIJ"
        "CgF4GAEgASgBEgkKAXkYAiABKAESCQoBehgDIAEoARIJCgF3GAQgASgBIjQKCE1hdHJpeFhkEgwKBHJvd3MYASABKA0SDAoEY29s"
        "cxgCIAEoDRIMCgRkYXRhGAMgAygBIjQKCE1hdHJpeFhmEgwKBHJvd3MYASABKA0SDAoEY29scxgCIAEoDRIMCgRkYXRhGAMgAygC"
        "IhgKCFZlY3RvclhmEgwKBGRhdGEYASADKAIiIAoIVmVjdG9yMmYSCQoBeBgBIAEoAhIJCgF5GAIgASgCYgZwcm90bzM=",
  };

  EXPECT_TRUE(saveSchemaToDatabase(service_definition, schema_db));

  // Retrieve schema names
  auto schema_names = retrieveSchemaNamesFromServiceId(service_definition.id, schema_db);
  EXPECT_EQ(schema_names.first, "heph.examples.types.proto.Pose");
  EXPECT_EQ(schema_names.second, "heph.examples.types.proto.Pose");

  // Retrieve an empty message for the schema we loaded.
  auto message = retrieveMessageFromDatabase("heph.examples.types.proto.Pose", schema_db);
  ASSERT_NE(message, nullptr);
  EXPECT_EQ(message->GetDescriptor()->name(), "Pose");
  EXPECT_TRUE(message->IsInitialized());

  // A message before parsing some bytes should not have any content.
  std::string json_output;
  auto status = google::protobuf::util::MessageToJsonString(*message, &json_output);
  EXPECT_TRUE(status.ok());
  std::cout << "Initial JSON output:  \n'''\n" << json_output << "\n'''" << std::endl;
  EXPECT_EQ(json_output, "{}");

  // Parse some bytes into the message.
  auto base64MessageByteString =
      "ChsJAAAAAAAA8D8RAAAAAAAAAEAZAAAAAAAACEASJAmamZmZmZm5PxGamZmZmZnJPxkzMzMzMzPTPyEAAAAAAADwPw==";

  auto message_bytes = foxglove::base64Decode(base64MessageByteString);
  EXPECT_TRUE(message->ParseFromArray(message_bytes.data(), static_cast<int>(message_bytes.size())));
  EXPECT_TRUE(message->IsInitialized());

  // Now the message should have some content.
  json_output.clear();
  status = google::protobuf::util::MessageToJsonString(*message, &json_output);
  EXPECT_TRUE(status.ok());
  std::cout << "JSON output after parsing bytes:  \n'''\n" << json_output << "\n'''" << std::endl;
  EXPECT_NE(json_output, "{}");

  // Now generate a random message for the schema.
  auto random_message = generateRandomMessageFromSchemaName("heph.examples.types.proto.Pose", schema_db);
  ASSERT_NE(random_message, nullptr);
  EXPECT_EQ(random_message->GetDescriptor()->name(), "Pose");

  // Now the message should have some content.
  json_output.clear();
  status = google::protobuf::util::MessageToJsonString(*random_message, &json_output);
  EXPECT_TRUE(status.ok());
  std::cout << "JSON output of randomizing the message: \n'''\n" << json_output << "\n'''" << std::endl;
  EXPECT_NE(json_output, "{}");
}

}  // namespace heph::ws_bridge::tests