//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/ipc_interface.h"

#include <cstdio>
#include <cstdlib>

#include <absl/log/check.h>
#include <hephaestus/ipc/zenoh/dynamic_subscriber.h>
#include <hephaestus/ipc/zenoh/liveliness.h>
#include <hephaestus/ipc/zenoh/raw_publisher.h>
#include <hephaestus/ipc/zenoh/raw_subscriber.h>
#include <hephaestus/ipc/zenoh/scout.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/serdes/dynamic_deserializer.h>
#include <hephaestus/utils/stack_trace.h>

namespace heph::ws {

IpcInterface::IpcInterface(std::shared_ptr<ipc::zenoh::Session> session, const ipc::zenoh::Config& config)
  : session_master_(session), config_(config) {
  CHECK(session_master_);

  {
    absl::MutexLock lock(&mutex_pub_);
    auto pub_config = config;
    pub_config.id = config.id.value() + "_pub";
    session_pub_ = ipc::zenoh::createSession(pub_config);
    CHECK(session_pub_);
  }

  {
    absl::MutexLock lock(&mutex_sub_);
    auto sub_config = config;
    sub_config.id = config.id.value() + "_sub";
    session_sub_ = ipc::zenoh::createSession(sub_config);
    CHECK(session_sub_);
  }

  {
    absl::MutexLock lock(&mutex_srv_);
    auto srv_config = config;
    srv_config.id = config.id.value() + "_srv";
    session_srv_ = ipc::zenoh::createSession(srv_config);
    CHECK(session_srv_);
  }
}

IpcInterface::~IpcInterface() {
  stop();
}

void IpcInterface::start() {
  fmt::println("[IPC Interface] - Starting...");

  // Nothing todo so far.

  fmt::println("[IPC Interface] - ONLINE");
}

void IpcInterface::stop() {
  fmt::println("[IPC Interface] - Stopping...");

  {
    absl::MutexLock lock(&mutex_sub_);
    subscribers_.clear();
  }

  {
    absl::MutexLock lock(&mutex_pub_);
    publishers_.clear();
  }

  {
    absl::MutexLock lock(&mutex_srv_);
    async_service_callbacks_.clear();
  }

  fmt::println("[IPC Interface] - OFFLINE");
}

bool IpcInterface::hasSubscriberImpl(const std::string& topic) const {
  return subscribers_.find(topic) != subscribers_.end();
}

bool IpcInterface::hasSubscriber(const std::string& topic) const {
  absl::MutexLock lock(&mutex_sub_);
  return hasSubscriberImpl(topic);
}

void IpcInterface::addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                                 TopicSubscriberWithTypeCallback subscriber_cb) {
  absl::MutexLock lock(&mutex_sub_);

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
      session_sub_, ipc::TopicConfig{ topic },
      [subscriber_cb, topic_type_info](const ipc::zenoh::MessageMetadata& metadata,
                                       std::span<const std::byte> data) {
        subscriber_cb(metadata, data, topic_type_info);
      },
      topic_type_info, subscriber_config);
}

void IpcInterface::removeSubscriber(const std::string& topic) {
  absl::MutexLock lock(&mutex_sub_);

  if (!hasSubscriberImpl(topic)) {
    heph::log(heph::FATAL, "[IPC Interface] - Subscriber for topic does not exist!", "topic", topic);
  }
  subscribers_.erase(topic);
}

void IpcInterface::callback_PublisherMatchingStatus(const std::string& topic,
                                                    const ipc::zenoh::MatchingStatus& status) {
  heph::log(heph::INFO, "[IPC Interface]: The topic has changed matching status!", "topic", topic, "matching",
            status.matching);
}

auto IpcInterface::callService(uint32_t call_id, const ipc::TopicConfig& topic_config,
                               std::span<const std::byte> buffer, std::chrono::milliseconds timeout)
    -> RawServiceResponses {
  (void)call_id;
  return ipc::zenoh::callServiceRaw(*session_srv_, topic_config, buffer, timeout);
}

void IpcInterface::callback_ServiceResponse(uint32_t call_id, const std::string& service_name,
                                            const RawServiceResponses& responses) {
  AsyncServiceResponseCallback callback;

  {
    absl::MutexLock lock(&mutex_srv_);
    auto it = async_service_callbacks_.find(call_id);
    if (it != async_service_callbacks_.end()) {
      fmt::println("[IPC Interface] - Forwarding service response (#{}) for service '{}' and call id [{}] to "
                   "bridge [ASYNC]",
                   responses.size(), service_name, call_id);

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

std::future<void> IpcInterface::callServiceAsync(uint32_t call_id, const ipc::TopicConfig& topic_config,
                                                 std::span<const std::byte> buffer,
                                                 std::chrono::milliseconds timeout,
                                                 AsyncServiceResponseCallback callback) {
  try {
    auto future = std::async(std::launch::async, [this, call_id, topic_config, buffer, timeout]() {
      CHECK(session_srv_);

      RawServiceResponses responses;

      try {
        // TODO(mfehr): This does currently not work / or rather it works, but not asynchronously.
        fmt::println("[IPC Interface] - Sending service request for service '{}' [ASYNC]", topic_config.name);

        responses = ipc::zenoh::callServiceRaw(*session_srv_, topic_config, buffer, timeout);
      } catch (const std::exception& e) {
        heph::log(heph::ERROR, "[IPC Interface] - Exception during async service call", "topic",
                  topic_config.name, "error", e.what());
        throw;
      }

      fmt::println("[IPC Interface] - Received service response (#{}) for service '{}' [ASYNC]",
                   responses.size(), topic_config.name);

      callback_ServiceResponse(call_id, topic_config.name, responses);
    });

    {
      absl::MutexLock lock(&mutex_srv_);
      async_service_callbacks_[call_id] = callback;
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

bool IpcInterface::hasPublisher(const std::string& topic) const {
  absl::MutexLock lock(&mutex_pub_);
  return publishers_.find(topic) != publishers_.end();
}

void IpcInterface::addPublisher(const std::string& topic, const serdes::TypeInfo& topic_type_info) {
  absl::MutexLock lock(&mutex_pub_);

  if (publishers_.find(topic) != publishers_.end()) {
    heph::log(heph::FATAL, "[IPC Interface] - Publisher for topic already exists!", "topic", topic);
  }

  auto publisher_config = ipc::zenoh::PublisherConfig{ .cache_size = std::nullopt,
                                                       .create_liveliness_token = true,
                                                       .create_type_info_service = true };

  publishers_[topic] = std::make_unique<ipc::zenoh::RawPublisher>(
      session_pub_, ipc::TopicConfig{ topic }, topic_type_info,
      [this, topic](const ipc::zenoh::MatchingStatus& status) {
        this->callback_PublisherMatchingStatus(topic, status);
      },
      publisher_config);
}

void IpcInterface::removePublisher(const std::string& topic) {
  absl::MutexLock lock(&mutex_pub_);

  if (publishers_.find(topic) == publishers_.end()) {
    heph::log(heph::FATAL, "[IPC Interface] - Publisher for topic does not exist!", "topic", topic);
  }

  publishers_.erase(topic);
}

bool IpcInterface::publishMessage(const std::string& topic, std::span<const std::byte> data) {
  absl::MutexLock lock(&mutex_pub_);

  if (publishers_.find(topic) == publishers_.end()) {
    heph::log(heph::FATAL, "[IPC Interface] - Publisher for topic does not exist!", "topic", topic);
  }

  return publishers_[topic]->publish(data);
}

}  // namespace heph::ws