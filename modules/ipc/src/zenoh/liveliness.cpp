//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/liveliness.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenoh/api/channels.hxx>
#include <zenoh/api/closures.hxx>
#include <zenoh/api/enums.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/reply.hxx>
#include <zenoh/api/sample.hxx>
#include <zenoh/api/subscriber.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {
namespace {
[[nodiscard]] auto toPublisherStatus(::zenoh::SampleKind kind) -> PublisherStatus {
  switch (kind) {
    case Z_SAMPLE_KIND_PUT:
      return PublisherStatus::ALIVE;
    case Z_SAMPLE_KIND_DELETE:
      return PublisherStatus::DROPPED;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable() in C++23
}

}  // namespace

auto getListOfPublishers(const Session& session, std::string_view topic) -> std::vector<PublisherInfo> {
  static constexpr auto FIFO_BOUND = 100;
  const ::zenoh::KeyExpr keyexpr(topic);
  auto replies = session.zenoh_session.liveliness_get(keyexpr, ::zenoh::channels::FifoChannel(FIFO_BOUND));

  std::set<std::string> topics;
  for (auto res = replies.recv(); std::holds_alternative<::zenoh::Reply>(res); res = replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (reply.is_ok()) {
      const auto& sample = reply.get_ok();
      topics.emplace(sample.get_keyexpr().as_string_view());
    } else {
      LOG(ERROR) << fmt::format("Invalid reply for liveliness on topic {}", topic);
    }
  }

  std::vector<PublisherInfo> infos;
  infos.reserve(topics.size());
  std::ranges::transform(topics, std::back_inserter(infos), [](const std::string& topic_name) {
    return PublisherInfo{ .topic = topic_name, .status = PublisherStatus::ALIVE };
  });
  return infos;
}

void printPublisherInfo(const PublisherInfo& info) {
  auto text = fmt::format("[Publisher] Topic: {}", info.topic);
  if (info.status == PublisherStatus::DROPPED) {
    text = fmt::format("{} - DROPPED", text);
  }

  fmt::println("{}", text);
}

PublisherDiscovery::PublisherDiscovery(SessionPtr session, TopicConfig topic_config /* = "**"*/,
                                       Callback&& callback)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , infos_consumer_(std::make_unique<concurrency::MessageQueueConsumer<PublisherInfo>>(
        [this](const PublisherInfo& info) { callback_(info); }, DEFAULT_CACHE_RESERVES)) {
  infos_consumer_->start();
  // NOTE: the liveliness token subscriber is called only when the status of the publisher changes.
  // This means that we won't get the list of publisher that are already running.
  // To do that we need to query the list of publisher beforehand.
  auto publishers_info = getListOfPublishers(*session_, topic_config_.name);
  for (const auto& info : publishers_info) {
    infos_consumer_->queue().forcePush(info);
  }

  // Here we create the subscriber for the liveliness tokens.
  // NOTE: If a publisher start publishing between the previous call and the time needed to start the
  // subscriber, we will loose that publisher.
  // This could be avoided by querying for the list of publisher after we start the subscriber and keeping a
  // track of what we already advertised so not to call the user callback twice on the same event.
  // TODO: implement the optimization described above.
  createLivelinessSubscriber();
}

PublisherDiscovery::~PublisherDiscovery() {
  auto stopped = infos_consumer_->stop();
  stopped.get();
  try {
    std::move(*liveliness_subscriber_).undeclare();
  } catch (std::exception& e) {
    LOG(ERROR) << fmt::format("Failed to undeclare liveliness subscriber for: {}. Exception: {}",
                              topic_config_.name, e.what());
  }
}

void PublisherDiscovery::createLivelinessSubscriber() {
  const ::zenoh::KeyExpr keyexpr(topic_config_.name);

  auto liveliness_callback = [this](const ::zenoh::Sample& sample) mutable {
    const PublisherInfo info{ .topic = std::string{ sample.get_keyexpr().as_string_view() },
                              .status = toPublisherStatus(sample.get_kind()) };

    infos_consumer_->queue().forcePush(info);
  };

  liveliness_subscriber_ =
      std::make_unique<::zenoh::Subscriber<void>>(session_->zenoh_session.liveliness_declare_subscriber(
          keyexpr, std::move(liveliness_callback), ::zenoh::closures::none));
}

}  // namespace heph::ipc::zenoh
