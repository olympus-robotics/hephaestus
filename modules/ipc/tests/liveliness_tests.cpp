//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <memory>
#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::tests {
namespace {

[[nodiscard]] auto generateRandomTopicName(std::mt19937_64& mt) -> std::string {
  static constexpr auto MAX_TOPIC_PARTS_COUNT = 5;
  static constexpr auto MAX_TOPIC_PART_LENGTH = 10;

  auto topic_parts_count = random::random<std::size_t>(mt, { .min = 1, .max = MAX_TOPIC_PARTS_COUNT });
  std::vector<std::string> topic_parts;
  std::generate_n(std::back_inserter(topic_parts), topic_parts_count,
                  [&mt]() { return random::random<std::string>(mt, MAX_TOPIC_PART_LENGTH, false, true); });
  return fmt::format("{}", fmt::join(topic_parts, "/"));
}

[[nodiscard]] auto generateSessionId(const std::optional<std::string>& session_id) -> ::zenoh::Id {
  auto config = Config{};
  config.id = session_id;
  auto session = createSession(config);

  return session->zenoh_session.get_zid();
}

TEST(Liveliness, TokenGeneration) {
  auto mt = random::createRNG();
  auto topic = generateRandomTopicName(mt);
  auto session_id = generateSessionId(std::nullopt);
  auto actor_type = random::random<EndpointType>(mt);

  auto keyexpr = generateLivelinessTokenKeyexpr(topic, session_id, actor_type);
  auto endpoint_info = parseLivelinessToken(keyexpr, ::zenoh::SampleKind::Z_SAMPLE_KIND_PUT);

  auto expected_endpoint_info = EndpointInfo{ .session_id = toString(session_id),
                                              .topic = topic,
                                              .type = actor_type,
                                              .status = EndpointInfo::Status::ALIVE };
  EXPECT_THAT(endpoint_info, Ne(std::nullopt));
  EXPECT_EQ(*endpoint_info, expected_endpoint_info);
}

TEST(Liveliness, TokenGenerationCustomSessionId) {
  static constexpr auto MAX_ID_LENGTH = 10;
  auto mt = random::createRNG();
  auto topic = generateRandomTopicName(mt);
  auto session_id = generateSessionId(random::random<std::string>(mt, MAX_ID_LENGTH, false, true));
  auto actor_type = random::random<EndpointType>(mt);

  auto keyexpr = generateLivelinessTokenKeyexpr(topic, session_id, actor_type);
  auto endpoint_info = parseLivelinessToken(keyexpr, ::zenoh::SampleKind::Z_SAMPLE_KIND_PUT);

  auto expected_endpoint_info = EndpointInfo{ .session_id = toString(session_id),
                                              .topic = topic,
                                              .type = actor_type,
                                              .status = EndpointInfo::Status::ALIVE };
  EXPECT_THAT(endpoint_info, Ne(std::nullopt));
  EXPECT_EQ(*endpoint_info, expected_endpoint_info);
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
