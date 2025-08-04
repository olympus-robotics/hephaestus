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

enum class RemoteNodeType : std::uint8_t { INPUT = 0, OUTPUT = 1 };

namespace internal {
template <typename Container, typename... Ts>
auto recv(heph::net::Socket& socket, Ts&&... ts) {
  auto message = Container{ std::forward<Ts>(ts)... };
  return stdexec::just(static_cast<std::uint16_t>(0)) |
         stdexec::let_value([&socket, message = std::move(message)](auto& size) {
           return heph::net::recvAll(socket, std::as_writable_bytes(std::span{ &size, 1 })) |
                  stdexec::let_value([&socket, &size, message = std::move(message)](auto /*unused*/) mutable {
                    message.resize(size);
                    return heph::net::recvAll(socket, std::as_writable_bytes(std::span{ message })) |
                           stdexec::then([&message](auto /*unused*/) { return std::move(message); });
                  });
         });
}
template <typename Container>
auto send(heph::net::Socket& socket, const Container& message) {
  if (message.size() > std::numeric_limits<std::uint16_t>::max()) {
    heph::panic("message too big ({})", message.size());
  }

  return stdexec::just(static_cast<std::uint16_t>(message.size())) |
         stdexec::let_value([&socket, &message](const std::uint16_t& size) {
           return heph::net::sendAll(socket, std::as_bytes(std::span{ &size, 1 })) |
                  stdexec::let_value([&socket, &message](auto /*unused*/) {
                    return heph::net::sendAll(socket, std::as_bytes(std::span{ message })) |
                           stdexec::then([](auto /*unused*/) {});
                  });
         });
}

template <typename T>
auto createNetEntity(const heph::net::Endpoint& endpoint, heph::concurrency::Context& context) {
  switch (endpoint.type()) {
#ifndef DISABLE_BLUETOOTH
    case heph::net::EndpointType::BT:
      return T::createL2cap(context);
#endif
    case heph::net::EndpointType::IPV4:
      return T::createTcpIpV4(context);
    case heph::net::EndpointType::IPV6:
      return T::createTcpIpV6(context);
    default:
      heph::panic("Unknown endpoint type");
  }
  __builtin_unreachable();
}

inline auto connect(heph::net::Socket& socket, const heph::net::Endpoint& endpoint,
                    const std::string& type_info, RemoteNodeType& type, const std::string& name) {
  return heph::net::connect(socket, endpoint) | stdexec::let_value([&] {
           return net::sendAll(socket, std::as_bytes(std::span{ &type, 1 })) |
                  stdexec::let_value([&](std::span<const std::byte> /*recv_buffer*/) {
                    return internal::send(socket, name) |
                           stdexec::let_value([&] { return internal::send(socket, type_info); });
                  }) |
                  stdexec::let_value([&socket] { return internal::recv<std::string>(socket); });
         });
}

inline auto sendMsg(heph::net::Socket& socket, std::string name, std::vector<std::byte> msg) {
  const auto msg_size = msg.size();
  if (msg_size > std::numeric_limits<std::uint16_t>::max()) {
    heph::panic("Message size too big");
  }

  return (stdexec::just(std::move(msg)) |
          stdexec::let_value([&socket](const auto& data) { return internal::send(socket, data); }) |
          stdexec::let_error([name = std::move(name)]<typename Error>(const Error& error) {
            if constexpr (std::is_same_v<std::error_code, Error>) {
              heph::log(heph::INFO, "Stop publishing", "node", name, "reason", error.message());

            } else {
              std::string reason;
              try {
                std::rethrow_exception(error);
              } catch (std::exception& exception) {
                reason = exception.what();

              } catch (...) {
                reason = "unknown exception";
              }
              heph::log(heph::INFO, "Stop publishing", "node", name, "reason", reason);
            }
            return stdexec::just_stopped();
          }));
}
}  // namespace internal

class RemoteInputSubscriberOperator {
public:
  using MsgT = std::pmr::vector<std::byte>;

  [[nodiscard]] auto name() const -> std::string {
    return fmt::format("{}/{}", socket_.remoteEndpoint(), name_);
  }

  RemoteInputSubscriberOperator(heph::net::Socket socket, std::string name)
    : socket_(std::move(socket)), name_(std::move(name)) {
  }

  auto trigger() {
    return internal::recv<std::pmr::vector<std::byte>>(socket_, &memory_resource_);
  }

private:
  heph::net::Socket socket_;
  std::string name_;
  std::pmr::unsynchronized_pool_resource memory_resource_;
};

template <typename T>
struct RemoteInputSubscriber : heph::conduit::Node<RemoteInputSubscriber<T>, RemoteInputSubscriberOperator> {
  static auto name(const RemoteInputSubscriber& self) {
    return self.data().name();
  }

  static auto trigger(RemoteInputSubscriber& self) {
    return self.data().trigger();
  }

  static auto execute(RemoteInputSubscriberOperator::MsgT msg) -> std::optional<T> {
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

class RemoteSubscriberOperator {
public:
  using MsgT = std::pmr::vector<std::byte>;

  explicit RemoteSubscriberOperator(heph::net::Endpoint endpoint, std::string name)
    : endpoint_(std::move(endpoint)), name_(std::move(name)) {
  }

  [[nodiscard]] auto name() const -> std::string {
    return fmt::format("{}/{}", endpoint_, name_);
  }

  auto trigger(heph::concurrency::Context* context, std::string* type_info) -> exec::task<MsgT>;

private:
private:
  RemoteNodeType type_{ RemoteNodeType::OUTPUT };
  std::optional<heph::net::Socket> socket_;
  heph::net::Endpoint endpoint_;
  std::pmr::unsynchronized_pool_resource memory_resource_;
  std::string name_;
  std::string type_info_;
  std::optional<std::string> last_error_;
};

inline auto RemoteSubscriberOperator::trigger(heph::concurrency::Context* context, std::string* type_info)
    -> exec::task<MsgT> {
  try {
    if (!socket_.has_value()) {
      socket_.emplace(internal::createNetEntity<heph::net::Socket>(endpoint_, *context));

      auto error = co_await internal::connect(*socket_, endpoint_, *type_info, type_, name_);
      if (error != "success") {
        heph::panic("Could not connect: {}", error);
      }
    }

    // NOLINTNEXTLINE (readability-static-accessed-through-instance)
    auto msg = co_await (internal::recv<std::pmr::vector<std::byte>>(socket_.value(), &memory_resource_) |
                         stdexec::upon_stopped([] { return std::pmr::vector<std::byte>{}; }));
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
    return internal::sendMsg(client_, name(), std::move(msg));
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
