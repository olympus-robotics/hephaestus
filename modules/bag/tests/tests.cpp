//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <memory>
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
#include "hephaestus/utils/filesystem/scoped_path.h"
#include "test_proto_conversion.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::bag::tests {
namespace {
constexpr std::size_t ROBOT_MSG_COUNT = 10;
constexpr auto ROBOT_MSG_PERIOD = std::chrono::milliseconds{ 1 };
constexpr std::size_t FLEET_MSG_COUNT = 5;
constexpr auto FLEET_MSG_PERIOD = std::chrono::milliseconds{ 2 };
constexpr auto SENDER_ID = "bag_tester";
constexpr auto ROBOT_TOPIC = "robot";
constexpr auto FLEET_TOPIC = "fleet";

[[nodiscard]] auto
createBag() -> std::tuple<utils::filesystem::ScopedPath, std::vector<Robot>, std::vector<Fleet>> {
  auto scoped_path = utils::filesystem::ScopedPath::createFile();
  auto mcap_writer = createMcapWriter({ scoped_path });

  auto robot_type_info = serdes::getSerializedTypeInfo<Robot>();
  mcap_writer->registerSchema(robot_type_info);
  mcap_writer->registerChannel(ROBOT_TOPIC, robot_type_info);

  auto fleet_type_info = serdes::getSerializedTypeInfo<Fleet>();
  mcap_writer->registerSchema(fleet_type_info);
  mcap_writer->registerChannel(FLEET_TOPIC, fleet_type_info);

  auto mt = random::createRNG();

  const auto start_time = std::chrono::nanoseconds{ 0 };
  std::vector<Robot> robots(ROBOT_MSG_COUNT);
  for (std::size_t i = 0; i < ROBOT_MSG_COUNT; ++i) {
    robots[i] = Robot::random(mt);
    mcap_writer->writeRecord({ .sender_id = SENDER_ID,
                               .topic = ROBOT_TOPIC,
                               .timestamp = start_time + i * ROBOT_MSG_PERIOD,
                               .sequence_id = i },
                             serdes::serialize(robots[i]));
  }

  std::vector<Fleet> fleet(FLEET_MSG_COUNT);
  for (std::size_t i = 0; i < FLEET_MSG_COUNT; ++i) {
    fleet[i] = Fleet::random(mt);
    mcap_writer->writeRecord({ .sender_id = SENDER_ID,
                               .topic = FLEET_TOPIC,
                               .timestamp = start_time + i * FLEET_MSG_PERIOD,
                               .sequence_id = i },
                             serdes::serialize(fleet[i]));
  }

  return { std::move(scoped_path), std::move(robots), std::move(fleet) };
}

TEST(Bag, PlayAndRecord) {
  auto output_bag = utils::filesystem::ScopedPath::createFile();
  auto [bag_path, robots, companies] = createBag();
  {
    auto reader = std::make_unique<mcap::McapReader>();
    const auto status = reader->open(bag_path);
    EXPECT_TRUE(status.ok());

    auto session = ipc::zenoh::createSession({});
    auto bag_writer = createMcapWriter({ output_bag });
    auto recorder = ZenohRecorder::create(
        { .session = session, .bag_writer = std::move(bag_writer), .topics_filter_params = {} });
    {
      auto player = ZenohPlayer::create(
          { .session = session, .bag_reader = std::move(reader), .wait_for_readers_to_connect = true });
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
  EXPECT_EQ(statistics->messageCount, ROBOT_MSG_COUNT + FLEET_MSG_COUNT);
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelCount, 2);
  const auto channels = reader->channels();
  EXPECT_THAT(channels, SizeIs(2));
  std::unordered_map<std::string, mcap::ChannelId> reverse_channels;  // NOLINT(misc-const-correctness)
  for (const auto& [id, channel] : channels) {
    reverse_channels[channel->topic] = id;
  }

  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelMessageCounts[reverse_channels[ROBOT_TOPIC]], ROBOT_MSG_COUNT);
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelMessageCounts[reverse_channels[FLEET_TOPIC]], FLEET_MSG_COUNT);

  auto read_options = mcap::ReadMessageOptions{};
  read_options.readOrder = mcap::ReadMessageOptions::ReadOrder::LogTimeOrder;
  auto messages = reader->readMessages([](const auto&) {}, read_options);

  for (const auto& message : messages) {
    if (message.channel->topic == ROBOT_TOPIC) {
      Robot robot;  // NOLINT(misc-const-correctness)
      serdes::deserialize({ message.message.data, message.message.dataSize }, robot);
      EXPECT_EQ(robot, robots[message.message.sequence]);
    } else if (message.channel->topic == FLEET_TOPIC) {
      Fleet fleet;  // NOLINT(misc-const-correctness)
      serdes::deserialize({ message.message.data, message.message.dataSize }, fleet);
      EXPECT_EQ(fleet, companies[message.message.sequence]);
    } else {
      FAIL() << "unexpected channel id: " << message.channel->topic;
    }
  }
}

}  // namespace

}  // namespace heph::bag::tests
