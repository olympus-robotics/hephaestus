//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include <stdexec/execution.hpp>

#include "hephaestus/conduit/forwarding_output.h"
#include "hephaestus/conduit/internal/never_stop.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::conduit {
template <typename T>
class ZenohPublisher {
public:
  template <std::size_t Capacity>
  explicit ZenohPublisher(Output<T, Capacity>& output, ipc::zenoh::SessionPtr session,
                          ipc::TopicConfig topic_config)
    : ZenohPublisher(output, "/zenoh" + output.name(), session, topic_config) {
  }

  explicit ZenohPublisher(ForwardingOutput<T>& output, ipc::zenoh::SessionPtr session,
                          ipc::TopicConfig topic_config)
    : ZenohPublisher(output, "/zenoh" + output.name(), session, topic_config) {
  }

private:
  template <typename Output>
  explicit ZenohPublisher(Output& output, std::string name, ipc::zenoh::SessionPtr session,
                          ipc::TopicConfig topic_config)
    : name_(std::move(name))
    , zenoh_publisher_(std::move(session), topic_config)
    , output_forwarder_(this, name_) {
    output.connect(output_forwarder_);
  }

private:
  class Input : public TypedInput<T> {
  public:
    Input(ZenohPublisher* self, const std::string& name) : TypedInput<T>(name), self_(self) {
    }

  private:
    auto doTrigger(SchedulerT /*scheduler*/) -> TypedInput<T>::SenderT final {
      return internal::NeverStop{};
    }
    void handleCompleted() final {
    }

    auto setValue(T t) -> TypedInput<T>::SetValueSenderT final {
      self_->zenoh_publisher_.publish(t);
      return stdexec::just();
    }

  private:
    ZenohPublisher* self_;
  };

  std::string name_;
  ipc::zenoh::Publisher<T> zenoh_publisher_;
  Input output_forwarder_;
};
}  // namespace heph::conduit
