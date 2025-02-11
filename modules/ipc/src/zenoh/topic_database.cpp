//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/topic_database.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc {
class ZenohTopicDatabase final : public ITopicDatabase {
public:
  explicit ZenohTopicDatabase(zenoh::SessionPtr session);
  ~ZenohTopicDatabase() override = default;

  [[nodiscard]] auto getTypeInfo(const std::string& topic) -> std::optional<serdes::TypeInfo> override;

  [[nodiscard]] auto getServiceTypeInfo(const std::string& topic)
      -> std::optional<serdes::ServiceTypeInfo> override;

private:
  zenoh::SessionPtr session_;

  absl::Mutex topic_mutex_;
  std::unordered_map<std::string, serdes::TypeInfo> topics_type_db_ ABSL_GUARDED_BY(topic_mutex_);

  absl::Mutex service_mutex_;
  std::unordered_map<std::string, serdes::ServiceTypeInfo>
      service_topics_type_db_ ABSL_GUARDED_BY(service_mutex_);
};

ZenohTopicDatabase::ZenohTopicDatabase(zenoh::SessionPtr session) : session_(std::move(session)) {
}

auto ZenohTopicDatabase::getTypeInfo(const std::string& topic) -> std::optional<serdes::TypeInfo> {
  {
    const absl::MutexLock lock{ &topic_mutex_ };
    if (topics_type_db_.contains(topic)) {
      return topics_type_db_[topic];
    }
  }  // Unlock while querying the service.

  auto query_topic = zenoh::getEndpointTypeInfoServiceTopic(topic);

  static constexpr auto TIMEOUT = std::chrono::milliseconds{ 5000 };
  const auto response =
      zenoh::callService<std::string, std::string>(*session_, TopicConfig{ query_topic }, "", TIMEOUT);
  if (response.empty()) {
    heph::log(heph::ERROR, "failed to get type info, no response from service", "topic", topic);
    return std::nullopt;
  }

  const absl::MutexLock lock{ &topic_mutex_ };
  // While waiting for the query someone else could have added the topic to the DB.
  if (!topics_type_db_.contains(topic)) {
    topics_type_db_[topic] = serdes::TypeInfo::fromJson(response.front().value);
  }

  return topics_type_db_[topic];
}

[[nodiscard]] auto ZenohTopicDatabase::getServiceTypeInfo(const std::string& topic)
    -> std::optional<serdes::ServiceTypeInfo> {
  {
    const absl::MutexLock lock{ &service_mutex_ };
    if (service_topics_type_db_.contains(topic)) {
      return service_topics_type_db_[topic];
    }
  }  // Unlock while querying the service.

  auto query_topic = zenoh::getEndpointTypeInfoServiceTopic(topic);

  static constexpr auto TIMEOUT = std::chrono::milliseconds{ 5000 };
  const auto response =
      zenoh::callService<std::string, std::string>(*session_, TopicConfig{ query_topic }, "", TIMEOUT);
  if (response.empty()) {
    heph::log(heph::ERROR, "failed to get type info, no response from service", "topic", topic);
    return std::nullopt;
  }

  const absl::MutexLock lock{ &service_mutex_ };
  // While waiting for the query someone else could have added the topic to the DB.
  if (!service_topics_type_db_.contains(topic)) {
    service_topics_type_db_[topic] = serdes::ServiceTypeInfo::fromJson(response.front().value);
  }

  return service_topics_type_db_[topic];
}

// ----------------------------------------------------------------------------------------------------------
auto createZenohTopicDatabase(zenoh::SessionPtr session) -> std::unique_ptr<ITopicDatabase> {
  return std::make_unique<ZenohTopicDatabase>(std::move(session));
}

}  // namespace heph::ipc
