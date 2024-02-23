//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <filesystem>

#include "eolo/serdes/serdes.h"
#define MCAP_IMPLEMENTATION
#define MCAP_COMPRESSION_NO_ZSTD
#define MCAP_COMPRESSION_NO_LZ4
#include <fmt/core.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <mcap/writer.hpp>

#include "eolo/bag/writer.h"
#include "eolo/examples/types/pose.h"
#include "eolo/examples/types_protobuf/pose.h"

auto main(int argc, const char* argv[]) -> int {
  if (argc != 2) {
    fmt::println(stderr, "Usage: {} <output.mcap>",
                 argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::exit(1);
  }
  std::filesystem::path output{ argv[1] };  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

  auto bag_writer = eolo::bag::createMcapWriter({ std::move(output) });

  auto type_info = eolo::serdes::getSerializedTypeInfo<eolo::examples::types::Pose>();
  bag_writer->registerSchema(type_info);
  bag_writer->registerChannel("pose", type_info);

  const auto start_time = std::chrono::system_clock::now().time_since_epoch();
  constexpr auto INTERVAL = std::chrono::nanoseconds{ 1000000 };
  constexpr uint32_t TOTAL_MSGS = 100;
  for (uint32_t i = 0; i < TOTAL_MSGS; ++i) {
    auto frame_time = start_time + i * INTERVAL;

    eolo::examples::types::Pose pose;
    pose.position = Eigen::Vector3d{ static_cast<double>(i), 2, 3 };
    auto data = eolo::serdes::serialize(pose);
    eolo::ipc::MessageMetadata metadata{
      .sender_id = "myself", .topic = "pose", .timestamp = frame_time, .sequence_id = i
    };

    bag_writer->writeRecord(metadata, data);
  }
}
