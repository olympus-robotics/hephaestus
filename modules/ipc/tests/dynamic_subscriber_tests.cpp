//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/ipc/zenoh/dynamic_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::tests {

TEST(DynamicSubscriber, StartStop) {
  auto params = DynamicSubscriberParams{
    .session = zenoh::createSession({}),
    .topics_filter_params = {},
    .init_subscriber_cb = [](const auto&, const auto&) {},
    .subscriber_cb = [](const auto&, const auto&, const auto&) {},
  };
  auto dynamic_subscriber = DynamicSubscriber(std::move(params));

  dynamic_subscriber.start().get();
  dynamic_subscriber.stop().get();
}

// TODO: improve on testing

}  // namespace heph::ipc::zenoh::tests
