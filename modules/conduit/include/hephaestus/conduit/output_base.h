//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string_view>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/node_base.h"

namespace heph::conduit {

struct OutputBase {
  explicit OutputBase(std::string_view name) : name_(name) {
  }
  virtual ~OutputBase() = default;
  [[nodiscard]] auto name() const -> std::string {
    if (node_ != nullptr) {
      return fmt::format("{}/{}", node_->name(), name_);
    }
    return std::string(name_);
  }
  virtual auto trigger() -> concurrency::AnySender<void> = 0;

  void setNode(NodeBase& node) {
    node_ = &node;
  }

private:
  NodeBase* node_{ nullptr };
  std::string_view name_;
};
}  // namespace heph::conduit
