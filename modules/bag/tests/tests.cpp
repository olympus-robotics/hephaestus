//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/bag/writer.h"
#include "hephaestus/bag/zenoh_player.h"
#include "hephaestus/bag/zenoh_recorder.h"
#include "hephaestus/random/random_generator.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/utils/filesystem/scoped_path.h"
#include "test.pb.h"
#include "test_proto_conversion.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::bag::tests {
namespace {
constexpr std::size_t USER_MSG_COUNT = 10;
constexpr auto USER_MSG_PERIOD = std::chrono::milliseconds{ 1 };
constexpr std::size_t COMPANY_MSG_COUNT = 5;
constexpr auto COMPANY_MSG_PERIOD = std::chrono::milliseconds{ 2 };
constexpr auto SENDER_ID = "bag_tester";
constexpr auto USER_TOPIC = "user";
constexpr auto COMPANY_TOPIC = "company";

[[nodiscard]] auto
createBag() -> std::tuple<utils::filesystem::ScopedPath, std::vector<User>, std::vector<Company>> {
  auto scoped_path = utils::filesystem::ScopedPath::createFile();
  auto mcap_writer = createMcapWriter({ scoped_path });

  auto user_type_info = serdes::getSerializedTypeInfo<User>();
  mcap_writer->registerSchema(user_type_info);
  mcap_writer->registerChannel(USER_TOPIC, user_type_info);

  auto company_type_info = serdes::getSerializedTypeInfo<Company>();
  mcap_writer->registerSchema(company_type_info);
  mcap_writer->registerChannel(COMPANY_TOPIC, company_type_info);

  auto mt = random::createRNG();

  const auto start_time = std::chrono::nanoseconds{ 0 };
  std::vector<User> users(USER_MSG_COUNT);
  for (std::size_t i = 0; i < USER_MSG_COUNT; ++i) {
    users[i] = User::random(mt);
    mcap_writer->writeRecord({ .sender_id = SENDER_ID,
                               .topic = USER_TOPIC,
                               .timestamp = start_time + i * USER_MSG_PERIOD,
                               .sequence_id = i },
                             serdes::serialize(users[i]));
  }

  std::vector<Company> company(COMPANY_MSG_COUNT);
  for (std::size_t i = 0; i < COMPANY_MSG_COUNT; ++i) {
    company[i] = Company::random(mt);
    mcap_writer->writeRecord({ .sender_id = SENDER_ID,
                               .topic = COMPANY_TOPIC,
                               .timestamp = start_time + i * COMPANY_MSG_PERIOD,
                               .sequence_id = i },
                             serdes::serialize(company[i]));
  }

  return { std::move(scoped_path), std::move(users), std::move(company) };
}

TEST(Bag, PlayAndRecord) {
  auto output_bag = utils::filesystem::ScopedPath::createFile();
  auto [bag_path, users, companies] = createBag();
  {
    auto reader = std::make_unique<mcap::McapReader>();
    const auto status = reader->open(bag_path);
    EXPECT_TRUE(status.ok());

    auto session = ipc::zenoh::createSession({});
    auto player = ZenohPlayer::create(
        { .session = session, .bag_reader = std::move(reader), .wait_for_readers_to_connect = true });

    auto bag_writer = createMcapWriter({ output_bag });
    auto recorder = ZenohRecorder::create(
        { .session = session, .bag_writer = std::move(bag_writer), .topics_filter_params = {} });

    recorder.start().get();
    player.start().get();
    player.wait();

    player.stop().get();
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
  EXPECT_EQ(statistics->messageCount, USER_MSG_COUNT + COMPANY_MSG_COUNT);
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelCount, 2);
  const auto channels = reader->channels();
  EXPECT_THAT(channels, SizeIs(2));
  std::unordered_map<std::string, mcap::ChannelId> reverse_channels;
  for (const auto& [id, channel] : channels) {
    reverse_channels[channel->topic] = id;
  }

  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelMessageCounts[reverse_channels[USER_TOPIC]], USER_MSG_COUNT);
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  EXPECT_EQ(statistics->channelMessageCounts[reverse_channels[COMPANY_TOPIC]], COMPANY_MSG_COUNT);

  auto read_options = mcap::ReadMessageOptions{};
  read_options.readOrder = mcap::ReadMessageOptions::ReadOrder::LogTimeOrder;
  auto messages = reader->readMessages([](const auto&) {}, read_options);

  for (const auto& message : messages) {
    if (message.channel->topic == USER_TOPIC) {
      User user;
      serdes::deserialize<User>({ message.message.data, message.message.dataSize }, user);
      EXPECT_EQ(user, users[message.message.sequence]);
    } else if (message.channel->topic == COMPANY_TOPIC) {
      Company company;
      serdes::deserialize<Company>({ message.message.data, message.message.dataSize }, company);
      EXPECT_EQ(company, companies[message.message.sequence]);
    } else {
      FAIL() << "unexpected channel id: " << message.channel->topic;
    }
  }
}

}  // namespace

}  // namespace heph::bag::tests
