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

#include "eolo/examples/types/proto/pose.pb.h"
#include "eolo/examples/types_protobuf/pose.h"

/// Builds a FileDescriptorSet of this descriptor and all transitive dependencies, for use as a
/// channel schema.
auto buildFileDescriptorSet(const google::protobuf::Descriptor* toplevel_descriptor)
    -> google::protobuf::FileDescriptorSet {
  google::protobuf::FileDescriptorSet fd_set;
  std::queue<const google::protobuf::FileDescriptor*> to_add;
  to_add.push(toplevel_descriptor->file());
  std::unordered_set<std::string> seen_dependencies;

  while (!to_add.empty()) {
    const google::protobuf::FileDescriptor* next = to_add.front();
    to_add.pop();
    next->CopyTo(fd_set.add_file());
    for (int i = 0; i < next->dependency_count(); ++i) {
      const auto& dep = next->dependency(i);
      if (seen_dependencies.find(dep->name()) == seen_dependencies.end()) {
        seen_dependencies.insert(dep->name());
        to_add.push(dep);
      }
    }
  }

  return fd_set;
}

template <class Clock>
auto toMcapTimestamp(const typename std::chrono::time_point<Clock>& time_point) -> mcap::Timestamp {
  return static_cast<mcap::Timestamp>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(time_point.time_since_epoch()).count());
}

auto main(int argc, const char* argv[]) -> int {
  if (argc != 2) {
    fmt::println(stderr, "Usage: {} <output.mcap>",
                 argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::exit(1);
  }
  std::filesystem::path output{ argv[1] };  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

  mcap::McapWriter writer;
  auto options = mcap::McapWriterOptions("");
  const auto res = writer.open(output.string(), options);
  if (!res.ok()) {
    fmt::println(stderr, "Failed to open: {} for writing", output.c_str());
    std::exit(1);
  }

  mcap::Schema pose_schema(
      "eolo.examples.types.proto.Pose", "protobuf",
      buildFileDescriptorSet(eolo::examples::types::proto::Pose::descriptor()).SerializeAsString());
  writer.addSchema(pose_schema);

  mcap::Channel pose_channel("pose", "protobuf", pose_schema.id);
  writer.addChannel(pose_channel);
  auto pose_channel_id = pose_channel.id;

  const auto start_time = toMcapTimestamp(std::chrono::system_clock::now());
  constexpr mcap::Timestamp INTERVAL = std::chrono::nanoseconds{ 1000000 }.count();
  constexpr uint32_t TOTAL_MSGS = 100;
  for (uint32_t i = 0; i < TOTAL_MSGS; ++i) {
    mcap::Timestamp frame_time = start_time + i * INTERVAL;

    eolo::examples::types::Pose pose;
    pose.position = Eigen::Vector3d{ static_cast<double>(i), 2, 3 };
    auto data = eolo::serdes::serialize(pose);
    mcap::Message msg;
    msg.channelId = pose_channel_id;
    msg.sequence = i;
    msg.publishTime = frame_time;
    msg.logTime = frame_time;
    msg.data = data.data();
    msg.dataSize = data.size();

    const auto write_res = writer.write(msg);
    if (!write_res.ok()) {
      fmt::println(stderr, "failed to write msg with error: {}", write_res.message);
      break;
    }
  }
}
