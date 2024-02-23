//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/topic_database.h"

#include <mutex>

#include <fmt/core.h>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/session.h"

namespace eolo::ipc {
class ZenohTopicDatabase final : public ITopicDatabase {
public:
  explicit ZenohTopicDatabase(zenoh::SessionPtr session);
  ~ZenohTopicDatabase() = default;

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
  fmt::println("Query service: {}", query_topic);
  const auto response = zenoh::query(session_->zenoh_session, query_topic, "");
  throwExceptionIf<eolo::InvalidDataException>(
      response.size() != 1,
      std::format("received {} responses for type from service {}", response.size(), query_topic));

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

}  // namespace eolo::ipc
