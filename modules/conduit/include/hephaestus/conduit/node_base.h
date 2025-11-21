//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

#include <fmt/format.h>

namespace heph::conduit {
namespace internal {}  // namespace internal
class NodeBase {
public:
  NodeBase(std::string prefix, std::string_view name, NodeBase* parent)
    : prefix_(std::move(prefix))
    , parent_(parent)
    , name_(resolveName(prefix_, name, parent_))
    , module_name_(prefix_.empty() ? name_ : name_.substr(prefix_.size() + 2)) {
  }
  virtual ~NodeBase() = default;

  [[nodiscard]] virtual auto name() const -> std::string {
    return name_;
  }
  virtual void enable() = 0;
  virtual void disable() = 0;

  [[nodiscard]] auto prefix() const -> const std::string& {
    return prefix_;
  }

  [[nodiscard]] auto moduleName() const -> const std::string& {
    return module_name_;
  }

private:
  static auto resolveName(const std::string& prefix, std::string_view name, NodeBase* parent) -> std::string {
    if (parent != nullptr) {
      return fmt::format("{}/{}", parent->name(), name);
    }

    if (prefix.empty()) {
      return fmt::format("/{}", name);
    }
    return fmt::format("/{}/{}", prefix, name);
  }

private:
  std::string prefix_;
  NodeBase* parent_{ nullptr };
  std::string name_;
  std::string module_name_;
};
}  // namespace heph::conduit
