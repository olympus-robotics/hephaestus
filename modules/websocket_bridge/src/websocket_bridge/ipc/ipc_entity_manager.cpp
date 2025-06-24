//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/ipc/ipc_entity_manager.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>

#include <absl/log/check.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/ipc/zenoh/raw_publisher.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"

namespace heph::ws {

IpcEntityManager::IpcEntityManager(std::shared_ptr<ipc::zenoh::Session> session, ipc::zenoh::Config config)
  : session_(std::move(session)), config_(std::move(config)) {
  CHECK(session_);
}

IpcEntityManager::~IpcEntityManager() {
  stop();
}

void IpcEntityManager::start() {
  heph::log(heph::INFO, "[IPC Interface] - Starting...");

  {
    const absl::MutexLock lock(&mutex_sub_);
    subscribers_.clear();
  }

  {
    const absl::MutexLock lock(&mutex_pub_);
    publishers_.clear();
  }

  {
    const absl::MutexLock lock(&mutex_srv_);
    async_service_callbacks_.clear();
  }

  heph::log(heph::INFO, "[IPC Interface] - ONLINE");
}

void IpcEntityManager::stop() {
  heph::log(heph::INFO, "[IPC Interface] - Stopping...");

  {
    const absl::MutexLock lock(&mutex_sub_);
    subscribers_.clear();
  }

  {
    const absl::MutexLock lock(&mutex_pub_);
    publishers_.clear();
  }

  {
    const absl::MutexLock lock(&mutex_srv_);
    async_service_callbacks_.clear();
  }

  heph::log(heph::INFO, "[IPC Interface] - OFFLINE");
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
bool IpcEntityManager::hasSubscriberImpl(const std::string& topic) const {
  return subscribers_.contains(topic);
}

auto IpcEntityManager::hasSubscriber(const std::string& topic) const -> bool {
  const absl::MutexLock lock(&mutex_sub_);
  return hasSubscriberImpl(topic);
}

void IpcEntityManager::addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                                     const TopicSubscriberWithTypeCallback& subscriber_cb) {
  const absl::MutexLock lock(&mutex_sub_);

  if (hasSubscriberImpl(topic)) {
    heph::log(heph::FATAL, "[IPC Interface] - Subscriber for topic already exists!", "topic", topic);
  }

  auto subscriber_config =
      ipc::zenoh::SubscriberConfig{ .cache_size = std::nullopt,
                                    .dedicated_callback_thread = true,
                                    // We do want to make the bridge subscriber discoverable.
                                    .create_liveliness_token = true,
                                    // We do not want this subscriber to advertise the type as it is anyways
                                    // only dyanmically derived/discovered, i.e. this subscriber only exists
                                    // if the publisher does.
                                    .create_type_info_service = false };

  subscribers_[topic] = std::make_unique<ipc::zenoh::RawSubscriber>(
      session_, ipc::TopicConfig{ topic },
      // NOLINTNEXTLINE(bugprone-exception-escape)
      [subscriber_cb, type_info = topic_type_info](const ipc::zenoh::MessageMetadata& metadata,
                                       std::span<const std::byte> data) {
        subscriber_cb(metadata, data, type_info);
      },
      topic_type_info, subscriber_config);
}

void IpcEntityManager::removeSubscriber(const std::string& topic) {
  const absl::MutexLock lock(&mutex_sub_);

  if (!hasSubscriberImpl(topic)) {
    heph::log(heph::FATAL, "[IPC Interface] - Subscriber for topic does not exist!", "topic", topic);
  }
  subscribers_.erase(topic);
}

void IpcEntityManager::publisherMatchingStatusCallback(const std::string& topic,
                                                       const ipc::zenoh::MatchingStatus& status) {
  heph::log(heph::INFO, "[IPC Interface]: The topic has changed matching status!", "topic", topic, "matching",
            status.matching);
}

auto IpcEntityManager::callService(uint32_t call_id, const ipc::TopicConfig& topic_config,
                                   std::span<const std::byte> buffer, std::chrono::milliseconds timeout)
    -> RawServiceResponses {
  (void)call_id;
  return ipc::zenoh::callServiceRaw(*session_, topic_config, buffer, timeout);
}

void IpcEntityManager::serviceResponseCallback(uint32_t call_id, const std::string& service_name,
                                               const RawServiceResponses& responses) {
  AsyncServiceResponseCallback callback;

  {
    const absl::MutexLock lock(&mutex_srv_);
    auto it = async_service_callbacks_.find(call_id);
    if (it != async_service_callbacks_.end()) {
      heph::log(heph::INFO, "[IPC Interface] - Forwarding service response to bridge [ASYNC]",
                "response_count", responses.size(), "service_name", service_name, "call_id", call_id);

      callback = it->second;
      async_service_callbacks_.erase(it);
    }
  }

  if (callback) {
    callback(responses);
  } else {
    heph::log(heph::ERROR, "[IPC Interface] - No callback found for service response", "service_name",
              service_name);
  }
}

auto IpcEntityManager::callServiceAsync(uint32_t call_id, const ipc::TopicConfig& topic_config,
                                        std::span<const std::byte> buffer, std::chrono::milliseconds timeout,
                                        AsyncServiceResponseCallback callback) -> std::future<void> {
  try {
    // NOLINTNEXTLINE(bugprone-exception-escape)
    auto future = std::async(std::launch::async, [this, call_id, topic_config, buffer, timeout]() {
      CHECK(session_);

      RawServiceResponses responses;

      try {
        heph::log(heph::INFO, "[IPC Interface] - Sending service request for service [ASYNC]", "service_name",
                  topic_config.name);

        responses = ipc::zenoh::callServiceRaw(*session_, topic_config, buffer, timeout);
      } catch (const std::exception& e) {
        heph::log(heph::ERROR, "[IPC Interface] - Exception during async service call", "topic",
                  topic_config.name, "error", e.what());
        throw;
      }

      heph::log(heph::INFO, "[IPC Interface] - Received service response [ASYNC]", "response_count",
                responses.size(), "service_name", topic_config.name);

      serviceResponseCallback(call_id, topic_config.name, responses);
    });

    {
      const absl::MutexLock lock(&mutex_srv_);
      async_service_callbacks_[call_id] = std::move(callback);
    }

    return future;
  } catch (const std::exception& e) {
    heph::log(heph::ERROR, "[IPC Interface] - Failed to dispatch async service call", "topic",
              topic_config.name, "error", e.what());
    std::promise<void> promise;
    promise.set_exception(std::make_exception_ptr(e));
    return promise.get_future();
  }
}

auto IpcEntityManager::hasPublisher(const std::string& topic) const -> bool {
  const absl::MutexLock lock(&mutex_pub_);
  return publishers_.contains(topic);
}

void IpcEntityManager::addPublisher(const std::string& topic, const serdes::TypeInfo& topic_type_info) {
  const absl::MutexLock lock(&mutex_pub_);

  panicIf(publishers_.contains(topic), "[IPC Interface] - Publisher for topic '{}' already exists!", topic);

  auto publisher_config = ipc::zenoh::PublisherConfig{ .cache_size = std::nullopt,
                                                       .create_liveliness_token = true,
                                                       .create_type_info_service = true };

  publishers_[topic] = std::make_unique<ipc::zenoh::RawPublisher>(
      session_, ipc::TopicConfig{ topic }, topic_type_info,
      // NOLINTNEXTLINE(bugprone-exception-escape)
      [topic](const ipc::zenoh::MatchingStatus& status) {
        IpcEntityManager::publisherMatchingStatusCallback(topic, status);
      },
      publisher_config);
}

void IpcEntityManager::removePublisher(const std::string& topic) {
  const absl::MutexLock lock(&mutex_pub_);

  panicIf(!publishers_.contains(topic), "[IPC Interface] - Publisher for topic '{}' does NOT already exists!",
          topic);

  publishers_.erase(topic);
}

auto IpcEntityManager::publishMessage(const std::string& topic, std::span<const std::byte> data) -> bool {
  const absl::MutexLock lock(&mutex_pub_);

  panicIf(!publishers_.contains(topic), "[IPC Interface] - Publisher for topic '{}' does NOT already exists!",
          topic);

  return publishers_[topic]->publish(data);
}

}  // namespace heph::ws
