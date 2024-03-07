//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/writer.h"

#include <algorithm>
#include <format>

#include <magic_enum.hpp>

#include "eolo/base/exception.h"
#include "eolo/ipc/common.h"
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
  explicit McapWriter(McapWriterParams params);
  ~McapWriter() override = default;

  void writeRecord(const ipc::MessageMetadata& metadata, std::span<const std::byte> data) override;
  void registerSchema(const serdes::TypeInfo& type_info) override;
  void registerChannel(const std::string& topic, const serdes::TypeInfo& type_info) override;

private:
private:
  McapWriterParams params_;
  mcap::McapWriter writer_;

  std::unordered_map<std::string, mcap::Schema> schema_db_;    /// Key is the type name.
  std::unordered_map<std::string, mcap::Channel> channel_db_;  /// Key is the topic.
};

McapWriter::McapWriter(McapWriterParams params) : params_(std::move(params)) {
  auto options = mcap::McapWriterOptions("");
  const auto status = writer_.open(params_.output_file.string(), options);
  throwExceptionIf<InvalidParameterException>(
      !status.ok(), std::format("failed to create Mcap writer for file {}, with error: {}",
                                params_.output_file.string(), status.message));
}

void McapWriter::registerSchema(const serdes::TypeInfo& type_info) {
  if (schema_db_.contains(type_info.name)) {
    return;
  }

  auto schema_type = serializationType(type_info.serialization);
  mcap::Schema schema(type_info.name, schema_type, type_info.schema);
  writer_.addSchema(schema);

  schema_db_[type_info.name] = schema;
}

void McapWriter::writeRecord(const ipc::MessageMetadata& metadata, std::span<const std::byte> data) {
  throwExceptionIf<InvalidDataException>(!channel_db_.contains(metadata.topic),
                                         std::format("no channel registered for topic {}", metadata.topic));

  auto channel_id = channel_db_[metadata.topic].id;

  mcap::Message msg;
  msg.channelId = channel_id;
  msg.sequence = static_cast<uint32_t>(metadata.sequence_id);
  msg.publishTime = static_cast<mcap::Timestamp>(metadata.timestamp.count());
  msg.logTime = static_cast<mcap::Timestamp>(metadata.timestamp.count());
  msg.data = data.data();
  msg.dataSize = data.size();

  const auto write_res = writer_.write(msg);
  throwExceptionIf<InvalidOperationException>(
      !write_res.ok(), std::format("failed to write msg from topic {} to bag", metadata.topic));
}

void McapWriter::registerChannel(const std::string& topic, const serdes::TypeInfo& type_info) {
  throwExceptionIf<InvalidDataException>(!schema_db_.contains(type_info.name),
                                         std::format("no schema registered for type {}", type_info.name));

  const auto& schema = schema_db_[type_info.name];
  mcap::Channel channel(topic, schema.encoding, schema.id);
  writer_.addChannel(channel);
  channel_db_[topic] = channel;
}

}  // namespace

auto createMcapWriter(McapWriterParams params) -> std::unique_ptr<IBagWriter> {
  return std::make_unique<McapWriter>(std::move(params));
}

}  // namespace eolo::bag
