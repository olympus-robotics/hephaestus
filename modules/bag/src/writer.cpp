//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/writer.h"

#include <algorithm>

#include <magic_enum.hpp>

#include "eolo/base/exception.h"
#include "eolo/serdes/type_info.h"

// TODO: add support for serialization
#define MCAP_IMPLEMENTATION
#define MCAP_COMPRESSION_NO_ZSTD
#define MCAP_COMPRESSION_NO_LZ4
#include <mcap/writer.hpp>

namespace eolo::bag {
namespace {

[[nodiscard]] auto serializationType(const serdes::TypeInfo::Serialization& serialization) -> std::string {
  auto schema_type = std::string{ magic_enum::enum_name(serialization) };
  std::transform(schema_type.begin(), schema_type.end(), schema_type.begin(),
                 [](char c) { return std::tolower(c); });
  return schema_type;
}

class McapWriter final : public IBagWriter {
public:
  explicit McapWriter(const McapWriterParams& params);
  ~McapWriter() = default;

  void writeRecord(const ipc::MessageMetadata& metadata, std::span<const std::byte> data) override;
  void registerSchema(const serdes::TypeInfo& type_info) override;

private:
  mcap::McapWriter writer_;
  std::unordered_map<std::string, serdes::TypeInfo> type_info_db_;
  std::unordered_map<std::string, mcap::SchemaId> schema_db_;
  std::unordered_map<std::string, mcap::ChannelId> channel_db_;
};

McapWriter::McapWriter(const McapWriterParams& params) {
  auto options = mcap::McapWriterOptions("");
  const auto status = writer_.open(params.output_file.string(), options);
  throwExceptionIf<InvalidParameterException>(
      !status.ok(), std::format("failed to create Mcap writer for file {}, with error: {}",
                                params.output_file.string(), status.message));
}

void McapWriter::registerSchema(const serdes::TypeInfo& type_info) {
  if (type_info_db_.contains(type_info.name)) {
    return;
  }

  auto schema_type = serializationType(type_info.serialization);
  mcap::Schema schema(type_info.name, schema_type, type_info.schema);
  writer_.addSchema(schema);

  type_info_db_[type_info.name] = type_info;
  schema_db_[type_info.name] = schema.id;
}

void McapWriter::writeRecord(const ipc::MessageMetadata& metadata, std::span<const std::byte> data) {
  (void)data;
  mcap::ChannelId channel_id = 0;
  if (channel_db_.contains(metadata.topic)) {
    channel_id = channel_db_[metadata.topic];
  } else {
    // auto schema_type = serializationType(metadata.serialization);
    // mcap::Channel pose_channel("pose", "protobuf", pose_schema.id);
    // writer.addChannel(pose_channel);
  }
  (void)channel_id;
}

}  // namespace

auto createMcapWriter() -> std::unique_ptr<IBagWriter> {
  return {};
}

}  // namespace eolo::bag
