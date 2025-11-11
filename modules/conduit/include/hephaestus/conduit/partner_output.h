//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <exec/task.hpp>
#include <hephaestus/concurrency/any_sender.h>
#include <hephaestus/concurrency/context.h>
#include <hephaestus/concurrency/repeat_until.h>
#include <hephaestus/serdes/serdes.h>
#include <hephaestus/utils/exception.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/channel.h"
#include "hephaestus/conduit/internal/net.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"

namespace heph::conduit {

struct PartnerOutputBase {
  virtual ~PartnerOutputBase() = default;
  auto sendData()
      //-> concurrency::AnySender<void>;
      -> exec::task<void>;

  [[nodiscard]] virtual auto partner() const -> const std::string& = 0;
  [[nodiscard]] virtual auto name() const -> const std::string& = 0;

  virtual auto getTypeInfo() -> const std::string& = 0;
  virtual auto getValue() -> concurrency::AnySender<std::vector<std::byte>> = 0;

  void charge(concurrency::Context* context, net::Endpoint endpoint) {
    context_ = context;
    endpoint_ = std::move(endpoint);
  }

private:
  std::shared_ptr<heph::net::Socket> client_;
  concurrency::Context* context_;
  net::Endpoint endpoint_;
};
template <typename T>
class PartnerOutput : public PartnerOutputBase {
public:
  explicit PartnerOutput(TypedInput<T>& input_base)
    : name_(input_base.name()), type_info_(serdes::getSerializedTypeInfo<T>().toJson()) {
  }

  auto setValue(T t) -> concurrency::AnySender<void> {
    if (output_ == nullptr) {
      return stdexec::just();
    }
    return output_->setValue(std::move(t));
  }
  [[nodiscard]] auto partner() const -> const std::string& final {
    return partner_;
  }
  [[nodiscard]] auto name() const -> const std::string& final {
    return resolved_name_;
  }
  auto getTypeInfo() -> const std::string& final {
    return type_info_;
  }
  auto getValue() -> concurrency::AnySender<std::vector<std::byte>> final {
    heph::panicIf(output_ == nullptr, "Output not set");

    return output_->getValue() | stdexec::then([](const T& t) { return serdes::serialize(t); });
  }

  auto setPartner(const std::string& prefix, std::string partner) -> PartnerOutputBase* {
    partner_ = std::move(partner);
    output_ = std::make_unique<concurrency::Channel<T, 1>>();
    resolved_name_ = resolveName(prefix);
    return this;
  }

private:
  auto resolveName(const std::string& prefix) {
    // add 2 to prefix size due to the `/` before and after it.
    return fmt::format("/{}/{}", partner_, name_.substr(prefix.size() + 2));
  }

private:
  std::string name_;
  std::string resolved_name_;
  std::string type_info_;
  std::string partner_;
  std::shared_ptr<concurrency::Channel<T, 1>> output_;
};

}  // namespace heph::conduit
