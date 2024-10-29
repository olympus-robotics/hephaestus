//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mcap/reader.hpp>
#include <mcap/types.hpp>

#include "hephaestus/bag/writer.h"
#include "hephaestus/bag/zenoh_player.h"
#include "hephaestus/bag/zenoh_recorder.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/utils/filesystem/scoped_path.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::bag::tests {
namespace {
constexpr std::size_t DUMMY_TYPE_MSG_COUNT = 10;
constexpr auto DUMMY_TYPE_MSG_PERIOD = std::chrono::milliseconds{ 1 };
constexpr std::size_t DUMMY_PRIMITIVE_TYPE_MSG_COUNT = 5;
constexpr auto DUMMY_PRIMITIVE_TYPE_MSG_PERIOD = std::chrono::milliseconds{ 2 };
constexpr auto SENDER_ID = "bag_tester";
constexpr auto DUMMY_TYPE_TOPIC = "bag_test/dummy_type";
constexpr auto DUMMY_PRIMITIVE_TYPE_TOPIC = "bag_test/dummy_primitive_type";

[[nodiscard]] auto createBag() -> std::tuple<utils::filesystem::ScopedPath, std::vector<types::DummyType>,
                                             std::vector<types::DummyPrimitivesType>> {
  auto scoped_path = utils::filesystem::ScopedPath::createFile();
  auto mcap_writer = createMcapWriter({ scoped_path });

  auto robot_type_info = serdes::getSerializedTypeInfo<types::DummyType>();
  mcap_writer->registerSchema(robot_type_info);
  mcap_writer->registerChannel(DUMMY_TYPE_TOPIC, robot_type_info);

  auto dummy_primitive_type_type_info = serdes::getSerializedTypeInfo<types::DummyPrimitivesType>();
  mcap_writer->registerSchema(dummy_primitive_type_type_info);
  mcap_writer->registerChannel(DUMMY_PRIMITIVE_TYPE_TOPIC, dummy_primitive_type_type_info);

  auto mt = random::createRNG();

  const auto start_time = std::chrono::nanoseconds{ 0 };
  std::vector<types::DummyType> dummy_types(DUMMY_TYPE_MSG_COUNT);
  for (std::size_t i = 0; i < DUMMY_TYPE_MSG_COUNT; ++i) {
    dummy_types[i] = types::DummyType::random(mt);
    mcap_writer->writeRecord({ .sender_id = SENDER_ID,
                               .topic = DUMMY_TYPE_TOPIC,
                               .timestamp = start_time + i * DUMMY_TYPE_MSG_PERIOD,
                               .sequence_id = i },
                             serdes::serialize(dummy_types[i]));
  }

  std::vector<types::DummyPrimitivesType> dummy_primitive_type(DUMMY_PRIMITIVE_TYPE_MSG_COUNT);
  for (std::size_t i = 0; i < DUMMY_PRIMITIVE_TYPE_MSG_COUNT; ++i) {
    dummy_primitive_type[i] = types::DummyPrimitivesType::random(mt);
    mcap_writer->writeRecord({ .sender_id = SENDER_ID,
                               .topic = DUMMY_PRIMITIVE_TYPE_TOPIC,
                               .timestamp = start_time + i * DUMMY_PRIMITIVE_TYPE_MSG_PERIOD,
                               .sequence_id = i },
                             serdes::serialize(dummy_primitive_type[i]));
  }

  return { std::move(scoped_path), std::move(dummy_types), std::move(dummy_primitive_type) };
}

// TODO: figure out how to isolate the network to make sure that only the two topics here are visible.
TEST(Bag, PlayAndRecord) {
  auto output_bag = utils::filesystem::ScopedPath::createFile();
  auto [bag_path, dummy_types, companies] = createBag();
  {
    auto bag_writer = createMcapWriter({ output_bag });
    auto recorder = ZenohRecorder::create({ .session = ipc::zenoh::createSession({}),
                                            .bag_writer = std::move(bag_writer),
                                            .topics_filter_params = {
                                                .include_topics_only = {},
                                                .prefix = "bag_test/",
                                                .exclude_topics = {},
                                            } });
    {
      auto reader = std::make_unique<mcap::McapReader>();
      const auto status = reader->open(bag_path);
      EXPECT_TRUE(status.ok());
      auto player = ZenohPlayer::create({ .session = ipc::zenoh::createSession({}),
                                          .bag_reader = std::move(reader),
                                          .wait_for_readers_to_connect = true });
      recorder.start().get();
      player.start().get();
      player.wait();

      player.stop().get();
    }

    recorder.stop().get();
  }

  auto reader = std::make_unique<mcap::McapReader>();
  const auto status = reader->open(output_bag);
  EXPECT_TRUE(status.ok());

  const auto summary_status = reader->readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);
  EXPECT_TRUE(summary_status.ok());

  auto statistics = reader->statistics();
  ASSERT_TRUE(statistics.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->messageCount, DUMMY_TYPE_MSG_COUNT + DUMMY_PRIMITIVE_TYPE_MSG_COUNT);
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelCount, 2);
  const auto channels = reader->channels();
  EXPECT_THAT(channels, SizeIs(2));

  std::unordered_map<std::string, mcap::ChannelId> reverse_channels;  // NOLINT(misc-const-correctness)
  for (const auto& [id, channel] : channels) {
    reverse_channels[channel->topic] = id;
  }

  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelMessageCounts[reverse_channels[DUMMY_TYPE_TOPIC]], DUMMY_TYPE_MSG_COUNT);
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelMessageCounts[reverse_channels[DUMMY_PRIMITIVE_TYPE_TOPIC]],
            DUMMY_PRIMITIVE_TYPE_MSG_COUNT);

  auto read_options = mcap::ReadMessageOptions{};
  read_options.readOrder = mcap::ReadMessageOptions::ReadOrder::LogTimeOrder;
  auto messages = reader->readMessages([](const auto&) {}, read_options);

  for (const auto& message : messages) {
    if (message.channel->topic == DUMMY_TYPE_TOPIC) {
      types::DummyType dummy_type;  // NOLINT(misc-const-correctness)
      serdes::deserialize({ message.message.data, message.message.dataSize }, dummy_type);
      EXPECT_EQ(dummy_type, dummy_types[message.message.sequence]);
    } else if (message.channel->topic == DUMMY_PRIMITIVE_TYPE_TOPIC) {
      types::DummyPrimitivesType dummy_primitive_type;  // NOLINT(misc-const-correctness)
      serdes::deserialize({ message.message.data, message.message.dataSize }, dummy_primitive_type);
      EXPECT_EQ(dummy_primitive_type, companies[message.message.sequence]);
    } else {
      FAIL() << "unexpected channel id: " << message.channel->topic;
    }
  }
}

}  // namespace

}  // namespace heph::bag::tests
