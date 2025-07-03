//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/net/connect.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/send.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/exception.h"

namespace heph::conduit {
namespace internal {
template <typename T>
auto createNetEntity(const heph::net::Endpoint& endpoint, heph::concurrency::Context& context) {
  switch (endpoint.type()) {
    case heph::net::EndpointType::BT:
      return T::createL2cap(context);
    case heph::net::EndpointType::IPV4:
      return T::createTcpIpV4(context);
    case heph::net::EndpointType::IPV6:
      return T::createTcpIpV6(context);
    default:
      heph::panic("Unknown endpoint type");
  }
  __builtin_unreachable();
}
}  // namespace internal

class RemoteSubscriberOperator {
  auto connect(std::string* type_info) {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return heph::net::connect(socket_.value(), endpoint_) |
           stdexec::let_value([this, size = static_cast<std::uint16_t>(name_.size())]() {
             return heph::net::sendAll(socket_.value(), std::as_bytes(std::span{ &size, 1 })) |
                    stdexec::let_value([this](auto /*unused*/) {
                      return heph::net::sendAll(socket_.value(), std::as_bytes(std::span{ name_ }));
                    });
           }) |
           stdexec::let_value(
               [this, type_info, size = static_cast<std::uint16_t>(type_info->size())](auto /*unused*/) {
                 return heph::net::sendAll(socket_.value(), std::as_bytes(std::span{ &size, 1 })) |
                        stdexec::let_value([this, type_info](auto /*unused*/) {
                          return heph::net::sendAll(socket_.value(), std::as_bytes(std::span{ *type_info }));
                        });
               });
  }

public:
  using MsgT = std::pmr::vector<std::byte>;

  explicit RemoteSubscriberOperator(heph::net::Endpoint endpoint, std::string name)
    : endpoint_(std::move(endpoint)), name_(std::move(name)) {
  }

  [[nodiscard]] auto name() const -> std::string {
    return fmt::format("{}/{}", endpoint_, name_);
  }

  auto trigger(heph::concurrency::Context* context, std::string* type_info) -> exec::task<MsgT> {
    try {
      if (!socket_.has_value()) {
        socket_.emplace(internal::createNetEntity<heph::net::Socket>(endpoint_, *context));

        co_await connect(type_info);
      }

      // NOLINTNEXTLINE (readability-static-accessed-through-instance)
      auto msg = co_await receiveData();
      if (!msg.empty()) {
        co_return msg;
      }
      heph::log(heph::ERROR, "Reconnecting subscriber, connection was closed", "node", name());

    } catch (std::exception& exception) {
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
    }

    socket_.reset();
    co_return {};
  }

private:
  auto receiveData() -> exec::task<MsgT> {
    std::uint16_t size = 0;
    auto size_buffer = std::as_writable_bytes(std::span{ &size, 1 });
    auto size_result =
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        co_await (heph::net::recvAll(socket_.value(), size_buffer) | stdexec::stopped_as_optional());
    if (!size_result.has_value()) {
      co_return {};
    }
    std::pmr::vector<std::byte> msg(size, &memory_resource_);
    auto msg_result =
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        co_await (heph::net::recvAll(socket_.value(), std::span{ msg }) | stdexec::stopped_as_optional());
    if (!msg_result.has_value()) {
      co_return {};
    }

    co_return msg;
  }

private:
  std::optional<heph::net::Socket> socket_;
  heph::net::Endpoint endpoint_;
  std::string name_;
  std::pmr::unsynchronized_pool_resource memory_resource_;
  std::string type_info_;
  std::optional<std::string> last_error_;
};

template <typename T>
struct RemoteSubscriberNode : heph::conduit::Node<RemoteSubscriberNode<T>, RemoteSubscriberOperator> {
  std::string type_info = heph::serdes::getSerializedTypeInfo<T>().toJson();

  static auto name(const RemoteSubscriberNode& self) {
    return self.data().name();
  }

  static auto trigger(RemoteSubscriberNode* self) {
    return self->data().trigger(&self->engine().scheduler().context(), &self->type_info);
  }

  static auto execute(RemoteSubscriberOperator::MsgT msg) -> std::optional<T> {
    auto msg_buffer = std::span{ msg };
    if (msg_buffer.empty()) {
      // No data received...
      return std::nullopt;
    }

    T value;
    heph::serdes::deserialize(msg_buffer, value);
    return value;
  }
};

class RemotePublisherOperator {
public:
  explicit RemotePublisherOperator(heph::net::Socket client, std::string name)
    : client_(std::move(client)), remote_endpoint_(client_.remoteEndpoint()), name_(std::move(name)) {
  }

  [[nodiscard]] auto name() const -> std::string {
    return fmt::format("{}/{}", remote_endpoint_, name_);
  }

  auto publish(std::vector<std::byte> msg) {
    const auto msg_size = msg.size();
    if (msg_size > std::numeric_limits<std::uint16_t>::max()) {
      heph::panic("Message size too big");
    }
    return stdexec::just(static_cast<std::uint16_t>(msg_size)) |
           stdexec::let_value([this](std::uint16_t& size) {
             auto size_buffer = std::as_bytes(std::span{ &size, 1 });
             return heph::net::sendAll(client_, size_buffer);
           }) |
           stdexec::let_value([this, msg = std::move(msg)](auto /*unused*/) {
             auto msg_buffer = std::span{ msg };
             return heph::net::sendAll(client_, msg_buffer) | stdexec::then([](auto /*unused*/) {});
           }) |
           stdexec::let_error([this]<typename Error>(const Error& error) {
             if constexpr (std::is_same_v<std::error_code, Error>) {
               heph::log(heph::INFO, "Stop publishing", "node", name(), "reason", error.message());

             } else {
               try {
                 std::rethrow_exception(error);
               } catch (std::exception& exception) {
                 heph::log(heph::INFO, "Stop publishing", "node", name(), "reason", exception.what());

               } catch (...) {
                 heph::log(heph::INFO, "Stop publishing", "node", name(), "reason", "unknown");
               }
             }
             return stdexec::just_stopped();
           });
  }

private:
  heph::net::Socket client_;
  heph::net::Endpoint remote_endpoint_;
  std::string name_;
};

template <typename T, typename InputPolicyT = InputPolicy<>>
struct RemotePublisherNode : conduit::Node<RemotePublisherNode<T, InputPolicyT>, RemotePublisherOperator> {
  QueuedInput<T, InputPolicyT> input{ this, "input" };

  static auto name(const RemotePublisherNode& self) {
    return self.data().name();
  }

  static auto trigger(RemotePublisherNode& self) {
    return self.input.get();
  }

  static auto execute(RemotePublisherNode& self, const T& t) {
    return self.data().publish(heph::serdes::serialize(t));
  }
};
}  // namespace heph::conduit
