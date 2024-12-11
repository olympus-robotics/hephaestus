//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/log_sink.h"

#include <thread>
#include <utility>

#include "hephaestus/utils/utils.h"

// Instead of deactivating the following check we could globally set AllowPartialMove to true
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
heph::telemetry::LogEntry::LogEntry(LogLevel level_in, MessageWithLocation&& message_in)
  : level{ level_in }
  , message{ std::move(message_in.value) }
  , location{ message_in.location }
  , thread_id{ std::this_thread::get_id() }
  , time{ LogEntry::ClockT::now() }
  , hostname{ heph::utils::getHostName() } {
}
