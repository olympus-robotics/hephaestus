//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/acceptor.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory_resource>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <absl/synchronization/mutex.h>
#include <exec/task.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/internal/net.h"
#include "hephaestus/conduit/partner_output.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"

namespace heph::conduit {
Acceptor::Acceptor(const AcceptorConfig& config) : partners_(config.partners) {
  acceptors_.reserve(config.endpoints.size());

  auto create_acceptor = [&](net::EndpointType type) {
    switch (type) {
#ifndef DISABLE_BLUETOOTH
      case heph::net::EndpointType::BT:
        return heph::net::Acceptor::createL2cap(context_);
#endif
      case heph::net::EndpointType::IPV4:
        return heph::net::Acceptor::createTcpIpV4(context_);
      case heph::net::EndpointType::IPV6:
        return heph::net::Acceptor::createTcpIpV6(context_);
      default:
        heph::panic("Unknown endpoint type");
    }
    __builtin_unreachable();
  };

  std::size_t i = 0;
  for (const auto& endpoint : config.endpoints) {
    acceptors_.push_back(create_acceptor(endpoint.type()));
    acceptors_.back().bind(endpoint);
    acceptors_.back().listen();
    scope_.spawn(acceptClient(i), concurrency::ContextEnv{ &context_ });
    ++i;
  }
  std::atomic<bool> started{ false };
  thread_ = std::thread{ [this, &started]() {
    context_.run([&started]() {
      started.store(true, std::memory_order_release);
      started.notify_all();
    });
  } };
  while (!started.load(std::memory_order_acquire)) {
    started.wait(false, std::memory_order_acquire);
  }
}

Acceptor::~Acceptor() {
  join();
}

void Acceptor::join() {
  stdexec::sync_wait(scope_.on_empty());
  context_.requestStop();
  if (thread_.joinable()) {
    thread_.join();
  }
}

auto Acceptor::endpoints() const -> std::vector<heph::net::Endpoint> {
  std::vector<heph::net::Endpoint> res;
  res.reserve(acceptors_.size());
  for (const auto& acceptor : acceptors_) {
    res.push_back(acceptor.localEndpoint());
  }
  return res;
}

void Acceptor::addPartner(const std::string& name, const heph::net::Endpoint& endpoint) {
  const absl::MutexLock lock{ &mutex_ };
  partners_.emplace(name, endpoint);
}

void Acceptor::requestStop() {
  stdexec::start_detached(stdexec::schedule(context_.scheduler()) |
                          stdexec::then([this]() { scope_.request_stop(); }));
}

auto Acceptor::acceptClient(std::size_t index) -> exec::task<void> {
  const heph::net::Acceptor& server = acceptors_[index];

  while (true) {
    try {
      auto client = co_await heph::net::accept(server);
      std::uint64_t type{};
      co_await heph::net::recvAll(client, std::as_writable_bytes(std::span{ &type, 1 }));
      scope_.spawn(handleClient(std::move(client), type));
    } catch (...) {
      exception_ = std::current_exception();
      co_return;
    }
  }
}

void Acceptor::spawn(const std::vector<PartnerOutputBase*>& outputs) {
  for (PartnerOutputBase* output : outputs) {
    {
      const absl::MutexLock lock{ &mutex_ };
      const auto& partner = output->partner();
      auto it = partners_.find(partner);
      if (it == partners_.end()) {
        fmt::println(stderr, "Partner not found {}", partner);
        return;
      }
      output->charge(&context_, it->second);
    }
    scope_.spawn(output->sendData());
  }
}

void Acceptor::setInputs(std::vector<BasicInput*> typed_inputs) {
  const absl::MutexLock lock{ &mutex_ };
  typed_inputs_.insert(typed_inputs_.end(), typed_inputs.begin(), typed_inputs.end());
}

auto Acceptor::handleClient(net::Socket client, std::uint64_t /*type*/) -> exec::task<void> {
  try {
    std::string name = co_await internal::recv<std::string>(client);
    std::string result = "SUCCESS";

    BasicInput* input{ nullptr };
    {
      const absl::MutexLock lock{ &mutex_ };
      auto it = std::ranges::find_if(
          typed_inputs_, [&name](BasicInput* basic_input) { return name == basic_input->name(); });
      if (it == typed_inputs_.end()) {
        result = fmt::format("ERROR: Could not find input {}. Available: [{}]", name,
                             fmt::join(typed_inputs_ | std::views::transform([](auto* basic_input) {
                                         return basic_input->name();
                                       }),
                                       ", "));
      } else {
        input = *it;
      }
    }
    if (input == nullptr) {
      co_await internal::send(client, result);
      co_return;
    }

    std::string type_info = co_await internal::recv<std::string>(client);

    if (type_info != input->getTypeInfo()) {
      result = fmt::format("ERROR: type info mismatch {} != {}", type_info, input->getTypeInfo());
      co_await internal::send(client, result);
      co_return;
    }

    co_await internal::send(client, result);

    std::pmr::unsynchronized_pool_resource resource;
    while (true) {
      auto buffer = co_await internal::recv<std::pmr::vector<std::byte>>(client, &resource);
      co_await input->setValue(buffer);
    }
  } catch (...) {
    fmt::println(stderr, "another error...");
  }
}
}  // namespace heph::conduit
