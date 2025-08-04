//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <optional>
#include <string>
#include <utility>

#include <exec/task.hpp>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

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
namespace detail {
template <typename T, typename InputPolicyT>
class SetRemoteInputOperator {
public:
  explicit SetRemoteInputOperator(heph::concurrency::Context* context, heph::net::Endpoint endpoint,
                                  std::string name)
    : context_(context), endpoint_(std::move(endpoint)), name_(std::move(name)) {
  }

  auto name() const {
    return fmt::format("{}/{}", endpoint_, name_);
  }

  auto execute(T t) -> exec::task<bool> {
    try {
      if (!socket_.has_value()) {
        socket_.emplace(internal::createNetEntity<heph::net::Socket>(endpoint_, *context_));

        auto error = co_await internal::connect(*socket_, endpoint_, type_info_, type_, name_);
        if (error != "success") {
          heph::panic("Could not connect: {}", error);
        }
      }

      auto msg = heph::serdes::serialize(t);
      co_await (internal::sendMsg(socket_.value(), name(), std::move(msg)) |
                stdexec::upon_stopped([this] { socket_.reset(); }));

    } catch (std::exception& exception) {
      socket_.reset();
      std::string error = exception.what();
      if (last_error_.has_value()) {
        if (*last_error_ == error) {
          error.clear();
        } else {
          last_error_.emplace(error);
        }
      } else {
        last_error_.emplace(error);
      }

      if (!error.empty()) {
        heph::log(heph::ERROR, "Retrying", "node", name(), "error", error);
      }
      co_return false;
    }
    co_return true;
  }

private:
  heph::concurrency::Context* context_;
  RemoteNodeType type_{ RemoteNodeType::INPUT };
  std::optional<heph::net::Socket> socket_;
  heph::net::Endpoint endpoint_;
  std::string name_;
  std::string type_info_ = heph::serdes::getSerializedTypeInfo<T>().toJson();
  std::optional<std::string> last_error_;
};

template <typename T, typename InputPolicyT>
struct SetRemoteInput : Node<SetRemoteInput<T, InputPolicyT>, SetRemoteInputOperator<T, InputPolicyT>> {
  QueuedInput<T, InputPolicyT> input{ this, "input" };

  static auto name(const SetRemoteInput& self) {
    return self.data().name();
  }

  static auto trigger(SetRemoteInput& self) {
    return self.input.get();
  }

  static auto execute(SetRemoteInput& self, T t) {
    return self.data().execute(std::move(t));
  }
};
}  // namespace detail

template <typename T, typename InputPolicyT = InputPolicy<>>
class RemoteInput {
public:
  explicit RemoteInput(NodeEngine& engine, heph::net::Endpoint endpoint, std::string name)
    : set_remote_input_(engine.createNode<detail::SetRemoteInput<T, InputPolicyT>>(
          &engine.scheduler().context(), std::move(endpoint), std::move(name))) {
  }

  template <typename Output>
  void connectTo(Output& output) {
    set_remote_input_->input.connectTo(output);
  }

  auto onComplete() -> auto& {
    return set_remote_input_;
  }

private:
  NodeHandle<detail::SetRemoteInput<T, InputPolicyT>> set_remote_input_;
};
}  // namespace heph::conduit
