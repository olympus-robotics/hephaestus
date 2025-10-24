//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/partner_output.h"

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

  while (true) {
    try {
      co_await stdexec::schedule(context_->scheduler());
      client_ = std::make_unique<heph::net::Socket>(
          internal::createNetEntity<heph::net::Socket>(*context_, endpoint_));
      auto response = co_await internal::connect(*client_, endpoint_, type_info, type, output_name);
      if (response != "SUCCESS") {
        fmt::println(stderr, "{}: {}", output_name, response);
        continue;
      }
      co_await sendDataImpl(this, client_);

    } catch (std::exception& e) {
      fmt::println(stderr, "{}: {}. Retrying", output_name, e.what());
    } catch (...) {
      fmt::println(stderr, "{}: Unkown error. Retrying", output_name);
    }
  }
}
}  // namespace heph::conduit
