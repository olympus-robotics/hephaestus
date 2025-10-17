//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/basic_input.h"

namespace heph::conduit {
BasicInput::BasicInput(std::string_view name) : name_(name) {
}

auto BasicInput::name() const -> std::string {
  if (node_ != nullptr) {
    return fmt::format("{}/{}", node_->name(), name_);
  }
  return std::string(name_);
}

[[nodiscard]] auto BasicInput::lastTriggerTime() const -> ClockT::time_point {
  return last_trigger_time_;
}
void BasicInput::updateTriggerTime() {
  last_trigger_time_ = ClockT::now();
}
void BasicInput::onCompleted() {
  updateTriggerTime();
  handleCompleted();
}
void BasicInput::handleError() {
}
void BasicInput::handleStopped() {
}
}  // namespace heph::conduit
