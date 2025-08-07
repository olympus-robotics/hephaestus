//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <fmt/format.h>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/node_handle.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/types_proto/bool.h"  // IWYU pragma: keep

namespace heph::conduit {
namespace internal {
class SetRemoteInputOperator {
public:
  explicit SetRemoteInputOperator(heph::concurrency::Context* context, heph::net::Endpoint endpoint,
                                  std::string name, bool reliable)
    : context_(context)
    , endpoint_(std::move(endpoint))
    , name_(std::move(name))
    , type_{ .type = RemoteNodeType::INPUT, .reliable = reliable } {
  }

  [[nodiscard]] auto name() const {
    return fmt::format("{}/{}", endpoint_, name_);
  }

  auto execute(std::vector<std::byte> msg, std::string* type_info) -> exec::task<bool>;

private:
  heph::concurrency::Context* context_;
  std::optional<heph::net::Socket> socket_;
  heph::net::Endpoint endpoint_;
  std::string name_;
  std::optional<std::string> last_error_;
  RemoteNodeType type_;
};

template <typename T, typename InputPolicyT>
struct SetRemoteInput : Node<SetRemoteInput<T, InputPolicyT>, SetRemoteInputOperator> {
  QueuedInput<T, InputPolicyT> input{ this, "input" };
  std::string type_info = heph::serdes::getSerializedTypeInfo<T>().toJson();

  static auto name(const SetRemoteInput& self) {
    return self.data().name();
  }

  static auto trigger(SetRemoteInput& self) {
    return self.input.get();
  }

  static auto execute(SetRemoteInput& self, const T& t) {
    return self.data().execute(heph::serdes::serialize(t), &self.type_info);
  }
};
}  // namespace internal

template <typename T, typename InputPolicyT = InputPolicy<>>
class RemoteInputPublisher {
public:
  explicit RemoteInputPublisher(NodeEngine& engine, heph::net::Endpoint endpoint, std::string name,
                                bool reliable = true)
    : set_remote_input_(engine.createNode<internal::SetRemoteInput<T, InputPolicyT>>(
          &engine.scheduler().context(), std::move(endpoint), std::move(name), reliable)) {
  }

  template <typename Output>
  void connectTo(Output& output) {
    set_remote_input_->input.connectTo(output);
  }

  auto onComplete() -> auto& {
    return set_remote_input_;
  }

private:
  NodeHandle<internal::SetRemoteInput<T, InputPolicyT>> set_remote_input_;
};
}  // namespace heph::conduit
