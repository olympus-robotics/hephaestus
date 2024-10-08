//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <utility>

#include <fmt/core.h>

#include "hephaestus/bag/writer.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/serdes/serdes.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    if (argc != 2) {
      fmt::println(stderr, "Usage: {} <output.mcap>",
                   argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      std::exit(1);
    }
    std::filesystem::path output{ argv[1] };  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    auto bag_writer = heph::bag::createMcapWriter({ std::move(output) });

    auto type_info = heph::serdes::getSerializedTypeInfo<heph::examples::types::Pose>();
    bag_writer->registerSchema(type_info);
    bag_writer->registerChannel("pose", type_info);

    const auto start_time = std::chrono::system_clock::now().time_since_epoch();
    constexpr auto INTERVAL = std::chrono::nanoseconds{ 1000000 };
    constexpr uint32_t TOTAL_MSGS = 100;
    for (uint32_t i = 0; i < TOTAL_MSGS; ++i) {
      auto frame_time = start_time + i * INTERVAL;

      heph::examples::types::Pose pose;
      pose.position = Eigen::Vector3d{ static_cast<double>(i), 2, 3 };  // NOLINT
      const auto data = heph::serdes::serialize(pose);
      const heph::ipc::zenoh::MessageMetadata metadata{
        .sender_id = "myself", .topic = "pose", .timestamp = frame_time, .sequence_id = i
      };

      bag_writer->writeRecord(metadata, data);
    }
  } catch (std::exception& e) {
    fmt::println("Failed with exception: {}", e.what());
    std::exit(1);
  }
}
