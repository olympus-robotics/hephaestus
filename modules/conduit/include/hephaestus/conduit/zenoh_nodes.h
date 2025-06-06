//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <utility>

#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"

namespace heph::conduit {

// template <typename T>
// struct ZenohSubscriberNode : conduit::Node<ZenohSubscriberNode<T>> {
//   ZenohSubscriberNode(ipc::zenoh::SessionPtr session, ipc::TopicConfig topic_config)
//     : input(this, topic_config.name)
//     , subscriber(std::move(session), std::move(topic_config),
//                  [this](const auto&, const auto& msg) { input.setValue(*msg); }) {
//   }

//   QueuedInput<T> input;
//   ipc::zenoh::Subscriber<T> subscriber;

//   static auto trigger(ZenohSubscriberNode& self) {
//     return self.input.get();
//   }

//   static auto execute(T value) -> T {
//     return std::move(value);
//   }
// };

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

template <typename T>
struct ZenohPublisherNode : conduit::Node<ZenohPublisherNode<T>, ZenohPublisherOperator<T>> {
  QueuedInput<T> input{ this, "input" };  // TODO(@fbrizzi): how can I set this based on the topic name?

  static auto trigger(ZenohPublisherNode& self) {
    return self.input.get();
  }

  static void execute(ZenohPublisherNode& self, T value) {
    self.data().publisher.publish(value);
  }
};

}  // namespace heph::conduit
