//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/log_sink.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/core.h>

#include "hephaestus/utils/utils.h"

namespace heph {
auto operator<<(std::ostream& os, const LogLevel& level) -> std::ostream& {
  switch (level) {
    case LogLevel::TRACE:
      os << "trace";
      break;
    case LogLevel::DEBUG:
      os << "debug";
      break;
    case LogLevel::INFO:
      os << "info";
      break;
    case LogLevel::WARN:
      os << "warn";
      break;
    case LogLevel::ERROR:
      os << "error";
      break;
    case LogLevel::FATAL:
      os << "fatal";
      break;
  }

  return os;
}
}  // namespace heph

namespace heph::telemetry {
// Instead of deactvating the following check we could globally set AllowPartialMove to true
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
LogEntry::LogEntry(LogLevel level_in, MessageWithLocation&& message_in)
  : level{ level_in }
  , message{ std::move(message_in.value) }
  , location{ message_in.location }
  , thread_id{ std::this_thread::get_id() }
  , time{ LogEntry::ClockT::now() }
  , hostname{ heph::utils::getHostName() } {
}

auto format(const LogEntry& log) -> std::string {
  std::stringstream ss;
  ss << "level=" << log.level;
  ss << " hostname=" << std::quoted(log.hostname);
  ss << " location="
     << std::quoted(fmt::format("{}:{}",
                                std::filesystem::path{ log.location.file_name() }.filename().string(),
                                log.location.line()));
  ss << " thread-id=" << log.thread_id;
  ss << " time=" << fmt::format("{:%Y-%m-%dT%H:%M:%SZ}", log.time);
  ss << " message=" << std::quoted(log.message);

  for (const Field<std::string>& field : log.fields) {
    ss << " " << field.key << "=" << field.value;
  }

  return ss.str();
}

}  // namespace heph::telemetry
