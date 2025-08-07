//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/utils/unique_function.h"

namespace heph::conduit {
class RemoteNodeHandler {
public:
  RemoteNodeHandler(concurrency::Context& context, const std::vector<heph::net::Endpoint>& endpoints,
                    std::exception_ptr& exception);

  ~RemoteNodeHandler();

  [[nodiscard]] auto endpoints() const -> std::vector<heph::net::Endpoint>;

  void run();
  void requestStop();

  template <typename Engine, typename Output>
  void registerOutput(Engine& engine, Output& output);
  template <typename Engine, typename Node>
  void registerImplicitOutput(Engine& engine, Node& node);
  template <typename Engine, typename InputT>
  void registerInput(Engine& engine, InputT* input);

private:
  template <typename ResultT, typename Engine, typename Output>
  void registerOutput(Engine& engine, Output& output, std::string name);

  auto acceptClients(std::size_t index) -> exec::task<void>;
  auto handleClient(heph::net::Socket client, RemoteNodeType type) -> exec::task<void>;

  template <typename T, typename Engine, typename Node>
  auto createPublisherNode(Engine& engine, Node& node, heph::net::Socket socket, std::string name,
                           bool reliable) {
    auto publisher = engine.template createNode<RemoteOutputPublisherNode<T>>(std::move(socket),
                                                                              std::move(name), reliable);
    publisher->input.connectTo(node);
  }

  template <typename T, typename Engine, typename InputT>
  auto createSubscriberNode(Engine& engine, InputT* input, heph::net::Socket socket, bool reliable) {
    auto subscriber =
        engine.template createNode<RemoteInputSubscriber<T>>(std::move(socket), input->name(), reliable);
    input->connectTo(subscriber);
  }

private:
  struct RegistryEntry {
    std::string type_info;
    heph::UniqueFunction<void(heph::net::Socket, bool)> factory;
  };

  std::exception_ptr& exception_;
  exec::async_scope scope_;

  std::vector<heph::net::Acceptor> acceptors_;
  std::unordered_map<std::string, RegistryEntry> client_handlers_;
};

template <typename Engine, typename Output>
void RemoteNodeHandler::registerOutput(Engine& engine, Output& output) {
  using ResultT = typename Output::ResultT;
  registerOutput<ResultT>(engine, output, output.name());
}

template <typename Engine, typename Node>
void RemoteNodeHandler::registerImplicitOutput(Engine& engine, Node& node) {
  using ExecuteOperationT = decltype(node.executeSender());
  using ExecuteResultVariantT = stdexec::value_types_of_t<ExecuteOperationT>;
  static_assert(std::variant_size_v<ExecuteResultVariantT> == 1);
  using ExecuteResultTupleT = std::variant_alternative_t<0, ExecuteResultVariantT>;

  if constexpr (std::tuple_size_v<ExecuteResultTupleT> == 1) {
    using ResultT = std::tuple_element_t<0, ExecuteResultTupleT>;
    registerOutput<ResultT>(engine, node, node.nodeName());
  }
}

template <typename ResultT, typename Engine, typename Output>
void RemoteNodeHandler::registerOutput(Engine& engine, Output& output, std::string name) {
  RegistryEntry entry;
  if constexpr (detail::ISOPTIONAL<ResultT>) {
    using ValueT = typename ResultT::value_type;
    entry =
        RegistryEntry{ .type_info = heph::serdes::getSerializedTypeInfo<ValueT>().toJson(),
                       .factory =
                           [this, &engine, &output, name](heph::net::Socket socket, bool reliable) {
                             createPublisherNode<ValueT>(engine, output, std::move(socket), name, reliable);
                           }

        };
  } else {
    if constexpr (!std::is_same_v<void, ResultT>) {
      entry =
          RegistryEntry{ .type_info = heph::serdes::getSerializedTypeInfo<ResultT>().toJson(),
                         .factory = [this, &engine, &output, name](heph::net::Socket socket, bool reliable) {
                           createPublisherNode<ResultT>(engine, output, std::move(socket), name, reliable);
                         } };
    }
    client_handlers_.emplace(name, std::move(entry));
  }
}

template <typename Engine, typename InputT>
void RemoteNodeHandler::registerInput(Engine& engine, InputT* input) {
  using ValueT = typename InputT::ValueT;
  client_handlers_.emplace(
      input->name(),
      RegistryEntry{ .type_info = heph::serdes::getSerializedTypeInfo<ValueT>().toJson(),
                     .factory = [this, &engine, input](heph::net::Socket socket, bool reliable) {
                       createSubscriberNode<ValueT>(engine, input, std::move(socket), reliable);
                     } });
}
}  // namespace heph::conduit
