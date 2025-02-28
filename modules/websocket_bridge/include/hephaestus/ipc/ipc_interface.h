//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <absl/log/check.h>
#include <absl/synchronization/mutex.h>
#include <hephaestus/ipc/zenoh/raw_publisher.h>
#include <hephaestus/ipc/zenoh/raw_subscriber.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/serdes/type_info.h>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws {

using TopicSubscriberWithTypeCallback = std::function<void(
    const ipc::zenoh::MessageMetadata&, std::span<const std::byte>, const serdes::TypeInfo&)>;

using RawServiceResponses = std::vector<ipc::zenoh::ServiceResponse<std::vector<std::byte>>>;
using AsyncServiceResponseCallback = std::function<void(const RawServiceResponses&)>;

class IpcInterface {
public:
  IpcInterface(std::shared_ptr<ipc::zenoh::Session> session, const ipc::zenoh::Config& config);

  void start();
  void stop();

  // Subsribers
  /////////////
  bool hasSubscriber(const std::string& topic) const;
  void addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                     TopicSubscriberWithTypeCallback callback);
  void removeSubscriber(const std::string& topic);

  // Publishers
  /////////////
  bool hasPublisher(const std::string& topic) const;
  void addPublisher(const std::string& topic, const serdes::TypeInfo& topic_type_info);
  void removePublisher(const std::string& topic);
  bool publishMessage(const std::string& topic, std::span<const std::byte> data);

  // Services
  ///////////
  auto callService(const ipc::TopicConfig& topic_config, std::span<const std::byte> buffer,
                   std::chrono::milliseconds timeout) -> RawServiceResponses;
  std::future<void> callServiceAsync(const ipc::TopicConfig& topic_config, std::span<const std::byte> buffer,
                                     std::chrono::milliseconds timeout,
                                     AsyncServiceResponseCallback callback);

private:
  bool hasSubscriberImpl(const std::string& topic) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_sub_);

  void callback_PublisherMatchingStatus(const std::string& topic, const ipc::zenoh::MatchingStatus& status);

  void callback_ServiceResponse(const std::string& service_name, const RawServiceResponses& responses);

  std::shared_ptr<ipc::zenoh::Session> session_master_;

  ipc::zenoh::Config config_;

  mutable absl::Mutex mutex_sub_;
  std::shared_ptr<ipc::zenoh::Session> session_sub_;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawSubscriber>>
      subscribers_ ABSL_GUARDED_BY(mutex_sub_);

  mutable absl::Mutex mutex_pub_;
  std::shared_ptr<ipc::zenoh::Session> session_pub_;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawPublisher>>
      publishers_ ABSL_GUARDED_BY(mutex_pub_);

  mutable absl::Mutex mutex_srv_;
  std::shared_ptr<ipc::zenoh::Session> session_srv_;
  std::unordered_map<std::string, AsyncServiceResponseCallback>
      async_service_callbacks_ ABSL_GUARDED_BY(mutex_srv_);
};

}  // namespace heph::ws