//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::bag {

class IBagWriter {
public:
  virtual ~IBagWriter() = default;

  virtual void registerSchema(const serdes::TypeInfo& type_info) = 0;
  virtual void registerChannel(const std::string& topic, const serdes::TypeInfo& type_info) = 0;
  virtual void writeRecord(const ipc::MessageMetadata& metadata, std::span<const std::byte> data) = 0;
};

struct McapWriterParams {
  std::filesystem::path output_file;
};

[[nodiscard]] auto createMcapWriter(McapWriterParams params) -> std::unique_ptr<IBagWriter>;

}  // namespace heph::bag
