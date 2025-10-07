//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/log_sink.h"

#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/telemetry/scope.h"
#include "hephaestus/utils/utils.h"

namespace heph::telemetry {
namespace {
[[nodiscard]] auto getModuleFromScope() -> std::string {
  const auto* current_scope = getCurrentScopeValue();
  return current_scope != nullptr ? fmt::format("/{}/{}", current_scope->robot_name, current_scope->module) :
                                    "global";
}
}  // namespace

// Instead of deactivating the following check we could globally set AllowPartialMove to true
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
LogEntry::LogEntry(LogLevel level_in, MessageWithLocation&& message_in)
  : level{ level_in }
  , message{ std::move(message_in.value) }
  , location{ message_in.location }
  , thread_id{ std::this_thread::get_id() }
  , time{ LogEntry::ClockT::now() }
  , hostname{ utils::getHostName() }
  , module{ getModuleFromScope() }
  , stack_trace{ std::nullopt } {
}
}  // namespace heph::telemetry
