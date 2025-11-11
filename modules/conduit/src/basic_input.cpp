//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/basic_input.h"

namespace heph::conduit {
BasicInput::BasicInput(std::string_view name) : name_(name) {
}

auto BasicInput::name() const -> std::string {
  if (node_ != nullptr) {
    return fmt::format("{}/inputs/{}", node_->name(), name_);
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
auto BasicInput::enabled() const -> bool {
  return enabled_.load(std::memory_order_acquire);
}
void BasicInput::enable() {
  enabled_.store(true, std::memory_order_release);
}
void BasicInput::disable() {
  enabled_.store(false, std::memory_order_release);
}
auto BasicInput::getIncoming() -> std::vector<std::string> {
  return {};
}
auto BasicInput::getOutgoing() -> std::vector<std::string> {
  return {};
}
auto BasicInput::setValue(const std::pmr::vector<std::byte>& /*buffer*/) -> concurrency::AnySender<void> {
  return stdexec::just();
}
[[nodiscard]] auto BasicInput::getTypeInfo() const -> std::string {
  abort();
  return "";
};
[[nodiscard]] auto BasicInput::trigger(SchedulerT scheduler) -> InputTrigger {
  return InputTrigger{ this, doTrigger(scheduler) };
}
}  // namespace heph::conduit
