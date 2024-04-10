//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/topic_database.h"

#include <mutex>

#include <fmt/core.h>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc {
class ZenohTopicDatabase final : public ITopicDatabase {
public:
  explicit ZenohTopicDatabase(zenoh::SessionPtr session);
  ~ZenohTopicDatabase() override = default;

  [[nodiscard]] auto getTypeInfo(const std::string& topic) -> const serdes::TypeInfo& override;

private:
  zenoh::SessionPtr session_;

  std::unordered_map<std::string, serdes::TypeInfo> topics_type_db_ ABSL_GUARDED_BY(mutex_);
  std::mutex mutex_;
};

ZenohTopicDatabase::ZenohTopicDatabase(zenoh::SessionPtr session) : session_(std::move(session)) {
}

auto ZenohTopicDatabase::getTypeInfo(const std::string& topic) -> const serdes::TypeInfo& {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (topics_type_db_.contains(topic)) {
      return topics_type_db_[topic];
    }
  }  // Unlock while querying the service.

  auto query_topic = getTypeInfoServiceTopic(topic);

  static constexpr auto TIMEOUT = std::chrono::milliseconds{ 500 };
  const auto response = zenoh::callService<std::string, std::string>(
      *session_, TopicConfig{ .name = query_topic }, "", TIMEOUT);
  throwExceptionIf<heph::InvalidDataException>(
      response.size() != 1,
      fmt::format("received no or too many responses for type from service {}", query_topic));

  std::unique_lock<std::mutex> lock(mutex_);
  // While waiting for the query someone else could have added the topic to the DB.
  if (!topics_type_db_.contains(topic)) {
    topics_type_db_[topic] = serdes::TypeInfo::fromJson(response.front().value);
  }

  return topics_type_db_[topic];
}

// ----------------------------------------------------------------------------------------------------------
auto createZenohTopicDatabase(std::shared_ptr<zenoh::Session> session) -> std::unique_ptr<ITopicDatabase> {
  return std::make_unique<ZenohTopicDatabase>(std::move(session));
}

}  // namespace heph::ipc
