#include "hephaestus/ipc/ipc_interface.h"

#include <cstdio>
#include <cstdlib>

#include <absl/log/check.h>
#include <hephaestus/ipc/zenoh/dynamic_subscriber.h>
#include <hephaestus/ipc/zenoh/liveliness.h>
#include <hephaestus/ipc/zenoh/scout.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/serdes/dynamic_deserializer.h>
#include <hephaestus/utils/stack_trace.h>

namespace heph::ws_bridge {

IpcInterface::IpcInterface(std::shared_ptr<ipc::zenoh::Session> session) : session_(session) {
}

void IpcInterface::start() {
  absl::MutexLock lock(&mutex_);

  fmt::println("[IPC Interface] - Starting...");

  // Nothing todo so far.

  fmt::println("[IPC Interface] - ONLINE");
}

void IpcInterface::stop() {
  absl::MutexLock lock(&mutex_);
  fmt::println("[IPC Interface] - Stopping...");

  subscribers_.clear();

  async_service_callbacks_.clear();

  fmt::println("[IPC Interface] - OFFLINE");
}

bool IpcInterface::hasSubscriberImpl(const std::string& topic) const {
  return subscribers_.find(topic) != subscribers_.end();
}

bool IpcInterface::hasSubscriber(const std::string& topic) const {
  absl::MutexLock lock(&mutex_);
  return hasSubscriberImpl(topic);
}

void IpcInterface::addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                                 TopicSubscriberWithTypeCallback subscriber_cb) {
  absl::MutexLock lock(&mutex_);

  if (hasSubscriberImpl(topic)) {
    heph::log(heph::FATAL, "[IPC Interface] - Subscriber for topic already exists!", "topic", topic);
  }

  auto subscriber_config =
      ipc::zenoh::SubscriberConfig{ .cache_size = std::nullopt,
                                    .dedicated_callback_thread = false,
                                    // We do want to make the bridge subscriber discoverable.
                                    .create_liveliness_token = true,
                                    // We do not want this subscriber to advertise the type as it is anyways
                                    // only dyanmically derived/discovered, i.e. this subscriber only exists
                                    // if the publisher does.
                                    .create_type_info_service = false };

  subscribers_[topic] = std::make_unique<ipc::zenoh::RawSubscriber>(
      session_, ipc::TopicConfig{ topic },
      [subscriber_cb, topic_type_info](const ipc::zenoh::MessageMetadata& metadata,
                                       std::span<const std::byte> data) {
        subscriber_cb(metadata, data, topic_type_info);
      },
      topic_type_info, subscriber_config);
}

void IpcInterface::removeSubscriber(const std::string& topic) {
  absl::MutexLock lock(&mutex_);

  if (!hasSubscriberImpl(topic)) {
    heph::log(heph::FATAL, "[IPC Interface] - Subscriber for topic does not exist!", "topic", topic);
  }
  subscribers_.erase(topic);
}

void IpcInterface::callback_PublisherMatchingStatus(const std::string& topic,
                                                    const zenoh::MatchingStatus& status) {
  heph::log(heph::INFO, "[IPC Interface]: The topic has changed matching status!", "topic", topic, "matching",
            status.matching);
}

auto IpcInterface::callService(const ipc::TopicConfig& topic_config, std::span<const std::byte> buffer,
                               std::chrono::milliseconds timeout) -> RawServiceResponses {
  return ipc::zenoh::callServiceRaw(*session_, topic_config, buffer, timeout);
}

std::future<void> IpcInterface::callServiceAsync(const ipc::TopicConfig& topic_config,
                                                 std::span<const std::byte> buffer,
                                                 std::chrono::milliseconds timeout,
                                                 AsyncServiceResponseCallback callback) {
  absl::MutexLock lock(&mutex_);

  try {
    auto future = std::async(std::launch::async, [this, topic_config, buffer, timeout]() {
      auto responses = ipc::zenoh::callServiceRaw(*session_, topic_config, buffer, timeout);
      absl::MutexLock lock(&mutex_);
      auto it = async_service_callbacks_.find(topic_config.name);
      if (it != async_service_callbacks_.end()) {
        it->second(responses);
        async_service_callbacks_.erase(it);
      }
    });

    async_service_callbacks_[topic_config.name] = callback;
    return future;
  } catch (const std::exception& e) {
    heph::log(heph::ERROR, "[IPC Interface] - Failed to dispatch async service call", "topic",
              topic_config.name, "error", e.what());
    std::promise<void> promise;
    promise.set_exception(std::make_exception_ptr(e));
    return promise.get_future();
  }
}

}  // namespace heph::ws_bridge