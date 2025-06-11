//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <string_view>
#include <utility>

#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/utils/string/string_literal.h"

namespace heph::conduit {

template <typename InputT>
class ZenohSubscriberNode {
public:
  ZenohSubscriberNode(ipc::zenoh::SessionPtr session, ipc::TopicConfig topic_config, InputT& input)
    : input_(&input)
    , subscriber_(std::move(session), std::move(topic_config),
                  [this](const auto&, const auto& msg) { input_->setValue(*msg); }) {
  }

private:
  InputT* input_;
  ipc::zenoh::Subscriber<typename InputT::DataT> subscriber_;
};

template <typename T>
struct ZenohPublisherOperator {
  ZenohPublisherOperator(ipc::zenoh::SessionPtr session, ipc::TopicConfig topic_config)
    : publisher(std::move(session), std::move(topic_config)) {
  }

  ipc::zenoh::Publisher<T> publisher;
};

template <typename T, utils::string::StringLiteral InputName>
struct ZenohPublisherNode : conduit::Node<ZenohPublisherNode<T, InputName>, ZenohPublisherOperator<T>> {
  QueuedInput<T> input{ this, std::string{ std::string_view{ InputName } } };

  static auto name() -> std::string_view {
    static constexpr auto NAME = utils::string::StringLiteral{ "zenoh_publisher:" } + InputName;
    return std::string_view{ NAME };
  }

  static auto trigger(ZenohPublisherNode& self) {
    return self.input.get();
  }

  static void execute(ZenohPublisherNode& self, T value) {
    self.data().publisher.publish(value);
  }
};

}  // namespace heph::conduit
