//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
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
#include <hephaestus/ipc/zenoh/dynamic_subscriber.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/serdes/type_info.h>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws_bridge {

using TopicSubscriberWithTypeCallback = std::function<void(
    const ipc::zenoh::MessageMetadata&, std::span<const std::byte>, const serdes::TypeInfo&)>;

using RawServiceResponses = std::vector<ipc::zenoh::ServiceResponse<std::vector<std::byte>>>;
using AsyncServiceResponseCallback = std::function<void(const RawServiceResponses&)>;

class IpcInterface {
public:
  IpcInterface(std::shared_ptr<ipc::zenoh::Session> session, const ipc::zenoh::Config& config);

  void start();
  void stop();

  bool hasSubscriber(const std::string& topic) const;
  void addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                     TopicSubscriberWithTypeCallback callback);
  void removeSubscriber(const std::string& topic);

  auto callService(const ipc::TopicConfig& topic_config, std::span<const std::byte> buffer,
                   std::chrono::milliseconds timeout) -> RawServiceResponses;

  std::future<void> callServiceAsync(const ipc::TopicConfig& topic_config, std::span<const std::byte> buffer,
                                     std::chrono::milliseconds timeout,
                                     AsyncServiceResponseCallback callback);

private:
  bool hasSubscriberImpl(const std::string& topic) const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void callback_PublisherMatchingStatus(const std::string& topic, const zenoh::MatchingStatus& status);

  void callback_ServiceResponse(const std::string& service_name, const RawServiceResponses& responses);

  std::shared_ptr<ipc::zenoh::Session> session_;
  ipc::zenoh::Config config_;

  mutable absl::Mutex mutex_;

  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawSubscriber>>
      subscribers_ ABSL_GUARDED_BY(mutex_);

  std::unordered_map<std::string, AsyncServiceResponseCallback> async_service_callbacks_;
};

}  // namespace heph::ws_bridge