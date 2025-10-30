//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/partner_output.h"

#include <chrono>
#include <coroutine>

namespace heph::conduit {
namespace {

auto sendDataImpl(PartnerOutputBase* output, std::shared_ptr<heph::net::Socket> socket) {
  return concurrency::repeatUntil([socket = std::move(socket), output]() mutable {
    return output->getValue() | stdexec::let_value([socket](const std::vector<std::byte>& buffer) {
             return internal::send(*socket, buffer) | stdexec::then([] { return false; });
           });
  });
}
}  // namespace

auto PartnerOutputBase::sendData()
    //-> concurrency::AnySender<void> {
    -> exec::task<void> {
  std::uint64_t type{ 0 };

  const auto& type_info = getTypeInfo();
  const auto& output_name = name();

  std::size_t attempt = 0;
  static constexpr std::chrono::seconds MAX_TIMEOUT{ 1 };
  static constexpr std::chrono::milliseconds TIMEOUT{ 2 };
  static constexpr double EXP = 1.5;
  while (true) {
    try {
      if (attempt == 0) {
        co_await stdexec::schedule(context_->scheduler());
      } else {
        auto timeout = std::chrono::milliseconds{ static_cast<std::size_t>(
            std::pow(attempt * TIMEOUT.count(), EXP * static_cast<double>(attempt))) };
        if (timeout >= MAX_TIMEOUT) {
          timeout = MAX_TIMEOUT;
        }
        co_await context_->scheduler().scheduleAfter(timeout);
      }
      client_ = std::make_unique<heph::net::Socket>(
          internal::createNetEntity<heph::net::Socket>(*context_, endpoint_));
      auto response = co_await internal::connect(*client_, endpoint_, type_info, type, output_name);
      if (response != "SUCCESS") {
        fmt::println(stderr, "{}: {}", output_name, response);
        ++attempt;
        continue;
      }
      co_await sendDataImpl(this, client_);

    } catch (std::exception& e) {
      ++attempt;
      fmt::println(stderr, "{}: {}. Retrying", output_name, e.what());
    } catch (...) {
      ++attempt;
      fmt::println(stderr, "{}: Unkown error. Retrying", output_name);
    }
    ++attempt;
  }
}
}  // namespace heph::conduit
