//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

#include <mcap/writer.hpp>

#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::bag {

class IBagWriter {
public:
  virtual ~IBagWriter() = default;

  virtual void registerSchema(const serdes::TypeInfo& type_info) = 0;
  virtual void registerChannel(const std::string& topic, const serdes::TypeInfo& type_info) = 0;
  virtual void writeRecord(const ipc::zenoh::MessageMetadata& metadata, std::span<const std::byte> data) = 0;
};

struct McapWriterParams {
  std::filesystem::path output_file;
  mcap::McapWriterOptions mcap_writer_options = mcap::McapWriterOptions("");
};

[[nodiscard]] auto createMcapWriter(McapWriterParams params) -> std::unique_ptr<IBagWriter>;

}  // namespace heph::bag
