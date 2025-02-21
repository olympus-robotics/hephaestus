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
  fmt::println("[IPC Interface] - Starting...");

  subscribers_.clear();

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

  subscribers_[topic] = std::make_unique<ipc::zenoh::RawSubscriber>(
      session_, ipc::TopicConfig{ topic },
      [subscriber_cb, topic_type_info](const ipc::zenoh::MessageMetadata& metadata,
                                       std::span<const std::byte> data) {
        subscriber_cb(metadata, data, topic_type_info);
      },
      topic_type_info);
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

}  // namespace heph::ws_bridge