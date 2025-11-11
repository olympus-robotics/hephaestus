//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"

namespace heph::conduit {
template <typename T>
class ZenohSubscriber {
public:
  explicit ZenohSubscriber(TypedInput<T>& input, ipc::zenoh::SessionPtr session,
                           ipc::TopicConfig topic_config)
    : input_(&input)
    , zenoh_subscriber_(std::move(session), topic_config,
                        [this](const auto&, const std::shared_ptr<T>& msg) { trigger(std::move(*msg)); }) {
  }

private:
  void trigger(T&& value) {
    stdexec::sync_wait(input_->setValue(std::move(value)));
  }

private:
  TypedInput<T>* input_;
  ipc::zenoh::Subscriber<T> zenoh_subscriber_;
};
}  // namespace heph::conduit
