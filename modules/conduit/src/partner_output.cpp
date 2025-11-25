//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/partner_output.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/conduit/internal/net.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log/log.h"

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

auto PartnerOutputBase::sendData() -> exec::task<void> {
  std::uint64_t type{ 0 };

  const auto& type_info = getTypeInfo();
  const auto& output_name = name();

  static constexpr std::size_t MAX_ATTEMPTS = 10;
  std::size_t attempt = 0;
  static constexpr std::chrono::minutes MAX_TIMEOUT{ 1 };
  static constexpr std::chrono::milliseconds TIMEOUT{ 2 };
  static constexpr double EXP = 1.5;
  while (true) {
    try {
      if (attempt == 0) {
        co_await stdexec::schedule(context_->scheduler());
      } else if (attempt == MAX_ATTEMPTS) {
        heph::panic("{}: max attempts reached", output_name);
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
        heph::log(heph::ERROR, "{}: {}", output_name, response);
        ++attempt;
        continue;
      }
      co_await sendDataImpl(this, client_);

    } catch (std::exception& e) {
      heph::log(heph::WARN, "Retrying.", "output", output_name, "error", e.what());
    } catch (...) {
      heph::log(heph::WARN, "Unknown error. Retrying", "output", output_name);
    }
    ++attempt;
  }
}
}  // namespace heph::conduit
