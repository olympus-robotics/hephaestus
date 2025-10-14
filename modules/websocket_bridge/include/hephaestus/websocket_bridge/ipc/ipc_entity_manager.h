//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/raw_publisher.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ws {

using TopicSubscriberWithTypeCallback = std::function<void(
    const ipc::zenoh::MessageMetadata&, std::span<const std::byte>, const serdes::TypeInfo&)>;

using RawServiceResponses = std::vector<ipc::zenoh::ServiceResponse<std::vector<std::byte>>>;
using AsyncServiceResponseCallback = std::function<void(const RawServiceResponses&)>;

class IpcEntityManager {
public:
  IpcEntityManager(std::shared_ptr<ipc::zenoh::Session> session, ipc::zenoh::Config config);

  ~IpcEntityManager();

  void start();
  void stop();

  // Subscribers
  /////////////
  [[nodiscard]] auto hasSubscriber(const std::string& topic) const -> bool;
  void addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                     const TopicSubscriberWithTypeCallback& subscriber_cb);
  void removeSubscriber(const std::string& topic);

  // Publishers
  /////////////
  [[nodiscard]] auto hasPublisher(const std::string& topic) const -> bool;
  void addPublisher(const std::string& topic, const serdes::TypeInfo& topic_type_info);
  void removePublisher(const std::string& topic);
  [[nodiscard]] auto publishMessage(const std::string& topic, std::span<const std::byte> data) -> bool;

  // Services
  ///////////
  [[nodiscard]] auto callService(uint32_t call_id, const ipc::TopicConfig& topic_config,
                                 std::span<const std::byte> buffer, std::chrono::milliseconds timeout)
      -> RawServiceResponses;
  auto callServiceAsync(uint32_t call_id, const ipc::TopicConfig& topic_config,
                        std::span<const std::byte> buffer, std::chrono::milliseconds timeout,
                        AsyncServiceResponseCallback callback) -> std::future<void>;

private:
  std::shared_ptr<ipc::zenoh::Session> session_;
  ipc::zenoh::Config config_;

  // Subscribers
  //////////////
  mutable absl::Mutex mutex_sub_;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawSubscriber>>
      subscribers_ ABSL_GUARDED_BY(mutex_sub_);

  // NOLINTNEXTLINE(modernize-use-trailing-return-type)
  [[nodiscard]] bool hasSubscriberImpl(const std::string& topic) const
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_sub_);

  // Publishers
  /////////////
  mutable absl::Mutex mutex_pub_;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawPublisher>>
      publishers_ ABSL_GUARDED_BY(mutex_pub_);

  static void publisherMatchingStatusCallback(const std::string& topic,
                                              const ipc::zenoh::MatchingStatus& status);

  // Services
  ///////////
  mutable absl::Mutex mutex_srv_;
  std::unordered_map<uint32_t, AsyncServiceResponseCallback>
      async_service_callbacks_ ABSL_GUARDED_BY(mutex_srv_);

  void serviceResponseCallback(uint32_t call_id, const std::string& service_name,
                               const RawServiceResponses& responses);
};

}  // namespace heph::ws
