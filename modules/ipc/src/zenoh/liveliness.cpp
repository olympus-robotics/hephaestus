//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/liveliness.h"

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/session.h"

namespace eolo::ipc::zenoh {
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

auto getListOfPublishers(const Session& session) -> std::vector<PublisherInfo> {
  static constexpr auto FIFO_BOUND = 100;

  z_owned_reply_channel_t channel = zc_reply_fifo_new(FIFO_BOUND);
  auto keyexpr = z_keyexpr(session.config.topic.c_str());
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

DiscoverPublishers::DiscoverPublishers(SessionPtr session, Callback&& callback)
  : session_(std::move(session)), callback_(std::move(callback)) {
  zenohc::ClosureSample cb = [this](const zenohc::Sample& sample) {
    PublisherInfo info{ .topic = std::string{ sample.get_keyexpr().as_string_view() },
                        .status = toPublisherStatus(sample.get_kind()) };
    this->callback_(info);
  };

  auto keyexpr = z_keyexpr(session_->config.topic.c_str());
  auto c = cb.take();
  liveliness_subscriber_ =
      zc_liveliness_declare_subscriber(session_->zenoh_session.loan(), keyexpr, z_move(c), nullptr);
  eolo::throwExceptionIf<eolo::FailedZenohOperation>(!z_check(liveliness_subscriber_),
                                                     "failed to create zenoh liveliness subscriber");
}

}  // namespace eolo::ipc::zenoh
