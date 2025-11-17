//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/node_base.h"
#include "hephaestus/conduit/scheduler.h"

namespace heph::conduit {

struct OutputBase {
  explicit OutputBase(std::string_view name) : name_(name) {
  }
  virtual ~OutputBase() = default;
  [[nodiscard]] auto name() const -> std::string {
    if (node_ != nullptr) {
      return fmt::format("{}/outputs/{}", node_->name(), name_);
    }
    return std::string(name_);
  }
  virtual auto trigger(SchedulerT scheduler) -> concurrency::AnySender<void> = 0;

  void setNode(NodeBase& node) {
    node_ = &node;
  }

  virtual auto getIncoming() -> std::vector<std::string> {
    return {};
  }

  virtual auto getOutgoing() -> std::vector<std::string> {
    return {};
  }

private:
  NodeBase* node_{ nullptr };
  std::string_view name_;
};
}  // namespace heph::conduit
