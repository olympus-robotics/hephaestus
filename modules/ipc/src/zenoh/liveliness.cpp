//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/liveliness.h"

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {
namespace {
[[nodiscard]] auto toPublisherStatus(zenohc::SampleKind kind) -> PublisherStatus {
  switch (kind) {
    case Z_SAMPLE_KIND_PUT:
      return PublisherStatus::ALIVE;
    case Z_SAMPLE_KIND_DELETE:
      return PublisherStatus::DROPPED;
  }
}
}  // namespace

auto getListOfPublishers(const Session& session, std::string_view topic) -> std::vector<PublisherInfo> {
  static constexpr auto FIFO_BOUND = 100;

  z_owned_reply_channel_t channel = zc_reply_fifo_new(FIFO_BOUND);
  auto keyexpr = z_keyexpr(topic.data());
  zc_liveliness_get(session.zenoh_session.loan(), keyexpr, z_move(channel.send), nullptr);
  z_owned_reply_t reply = z_reply_null();

  std::vector<PublisherInfo> infos;
  for (z_call(channel.recv, &reply); z_check(reply); z_call(channel.recv, &reply)) {
    if (z_reply_is_ok(&reply)) {
      auto sample = static_cast<zenohc::Sample>(z_reply_ok(&reply));
      infos.emplace_back(std::string{ sample.get_keyexpr().as_string_view() }, PublisherStatus::ALIVE);
    } else {
      fmt::println("Received an error");
    }
  }

  z_drop(z_move(reply));
  z_drop(z_move(channel));

  return infos;
}

void printPublisherInfo(const PublisherInfo& info) {
  auto text = std::format("[Publisher] Topic: {}", info.topic);
  if (info.status == PublisherStatus::DROPPED) {
    text = std::format("{} - DROPPED", text);
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
  zenohc::ClosureSample cb = [this](const zenohc::Sample& sample) {
    PublisherInfo info{ .topic = std::string{ sample.get_keyexpr().as_string_view() },
                        .status = toPublisherStatus(sample.get_kind()) };
    this->callback_(info);
  };

  auto keyexpr = z_keyexpr(topic_config_.name.data());
  auto c = cb.take();
  liveliness_subscriber_ =
      zc_liveliness_declare_subscriber(session_->zenoh_session.loan(), keyexpr, z_move(c), nullptr);
  heph::throwExceptionIf<heph::FailedZenohOperation>(!z_check(liveliness_subscriber_),
                                                     "failed to create zenoh liveliness subscriber");
}

}  // namespace heph::ipc::zenoh
