//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/writer.h"

#include <algorithm>
#include <format>

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

  void writeRecord(const RecordMetadata& metadata, std::span<const std::byte> data) override;
  void registerSchema(const serdes::TypeInfo& type_info) override;

private:
  [[nodiscard]] auto registerChannel(const std::string& topic, const std::string& type) -> mcap::ChannelId;

  mcap::McapWriter writer_;

  std::unordered_map<std::string, mcap::Schema> schema_db_;    /// Key is the type name.
  std::unordered_map<std::string, mcap::Channel> channel_db_;  /// Key is the topic.
};

McapWriter::McapWriter(const McapWriterParams& params) {
  auto options = mcap::McapWriterOptions("");
  const auto status = writer_.open(params.output_file.string(), options);
  throwExceptionIf<InvalidParameterException>(
      !status.ok(), std::format("failed to create Mcap writer for file {}, with error: {}",
                                params.output_file.string(), status.message));
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

void McapWriter::writeRecord(const RecordMetadata& metadata, std::span<const std::byte> data) {
  (void)data;

  mcap::ChannelId channel_id = 0;
  if (channel_db_.contains(metadata.metadata.topic)) {
    channel_id = channel_db_[metadata.metadata.topic].id;
  } else {
    channel_id = registerChannel(metadata.metadata.topic, metadata.type_info.name);
  }

  mcap::Message msg;
  msg.channelId = channel_id;
  msg.sequence = static_cast<uint32_t>(metadata.metadata.sequence_id);
  msg.publishTime = static_cast<mcap::Timestamp>(metadata.metadata.timestamp.count());
  msg.logTime = static_cast<mcap::Timestamp>(metadata.metadata.timestamp.count());
  msg.data = data.data();
  msg.dataSize = data.size();

  const auto write_res = writer_.write(msg);
  throwExceptionIf<InvalidOperationException>(
      !write_res.ok(), std::format("failed to write msg from topic {} to bag", metadata.metadata.topic));
}

auto McapWriter::registerChannel(const std::string& topic, const std::string& type) -> mcap::ChannelId {
  throwExceptionIf<InvalidDataException>(
      schema_db_.contains(type),
      std::format("cannot write record from topic {}, type {} as I don't have an associated schema", topic,
                  type));
  const auto& schema = schema_db_[type];
  mcap::Channel channel(topic, schema.encoding, schema.id);
  writer_.addChannel(channel);
  channel_db_[topic] = channel;
  return channel.id;
}

}  // namespace

auto createMcapWriter() -> std::unique_ptr<IBagWriter> {
  return {};
}

}  // namespace eolo::bag
