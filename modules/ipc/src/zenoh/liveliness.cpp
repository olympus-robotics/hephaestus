//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/liveliness.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenoh.hxx>
#include <zenoh_macros.h>

#include "hephaestus/ipc/common.h"
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

  std::vector<PublisherInfo> infos;
  for (auto res = replies.recv(); std::holds_alternative<::zenoh::Reply>(res); res = replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (reply.is_ok()) {
      const auto& sample = reply.get_ok();
      infos.emplace_back(std::string{ sample.get_keyexpr().as_string_view() }, PublisherStatus::ALIVE);
    } else {
      LOG(ERROR) << fmt::format("Invalid reply for liveliness on topic {}", topic);
    }
  }

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
  : session_(std::move(session)), topic_config_(std::move(topic_config)), callback_(std::move(callback)) {
  // NOTE: the liveliness token subscriber is called only when the status of the publisher changes.
  // This means that we won't get the list of publisher that are already running.
  // To do that we need to query the list of publisher beforehand.
  auto publishers_info = getListOfPublishers(*session_, topic_config_.name);

  // Here we create the subscriber for the liveliness tokens.
  // NOTE: If a publisher start publishing between the previous call and the time needed to start the
  // subscriber, we will loose that publisher.
  // This could be avoided by querying for the list of publisher after we start the subscriber and keeping a
  // track of what we already advertised so not to call the user callback twice on the same event.
  // TODO: implement the optimization described above.
  createLivelinessSubscriber();

  // We call the user callback after we setup the subscriber to minimize the chances of missing publishers.
  for (const auto& info : publishers_info) {
    this->callback_(info);
  }
}

void PublisherDiscovery::createLivelinessSubscriber() {
  const ::zenoh::KeyExpr keyexpr(topic_config_.name);
  auto callback = [this](const ::zenoh::Sample& sample) {
    const PublisherInfo info{ .topic = std::string{ sample.get_keyexpr().as_string_view() },
                              .status = toPublisherStatus(sample.get_kind()) };
    this->callback_(info);
  };

  liveliness_subscriber_ =
      std::make_unique<::zenoh::Subscriber<void>>(session_->zenoh_session.liveliness_declare_subscriber(
          keyexpr, std::move(callback), ::zenoh::closures::none));
}

}  // namespace heph::ipc::zenoh
