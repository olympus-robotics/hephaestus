//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/telemetry/struclog.h"

#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <source_location>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <unistd.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/utils.h"

namespace heph::telemetry {
auto operator<<(std::ostream& os, const Level& level) -> std::ostream& {
  switch (level) {
    case Level::TRACE:
      os << "trace";
      break;
    case Level::DEBUG:
      os << "debug";
      break;
    case Level::INFO:
      os << "info";
      break;
    case Level::WARN:
      os << "warn";
      break;
    case Level::ERROR:
      os << "error";
      break;
    case Level::FATAL:
      os << "fatal";
      break;
  }

  return os;
}

class Logger final {
public:
  explicit Logger();
  ~Logger();

  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  auto operator=(const Logger&) -> Logger& = delete;
  auto operator=(Logger&&) -> Logger& = delete;

  static void registerSink(std::unique_ptr<ILogSink> sink);

  static void log(const LogEntry& log_entry);

private:
  [[nodiscard]] static auto instance() -> Logger&;

  /// @brief Do the actual logging. Take in LogEntry and send it to all sinks
  /// @param LogEntry, class that takes in the structured logs and formats them.
  /// @return void
  void processLogEntries(const LogEntry& entry);

private:
  std::mutex sink_mutex_;
  // This is used as registry for the log sinks
  std::vector<std::unique_ptr<ILogSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  concurrency::MessageQueueConsumer<LogEntry> log_entries_consumer_;
};

void registerLogSink(std::unique_ptr<ILogSink> sink) {
  Logger::registerSink(std::move(sink));
}

void log(const LogEntry& log_entry) {
  Logger::log(log_entry);
}

Logger::Logger()
  : log_entries_consumer_([this](const LogEntry& entry) { processLogEntries(entry); }, std::nullopt) {
  log_entries_consumer_.start();
}

Logger::~Logger() {
  log_entries_consumer_.stop();
}

auto Logger::instance() -> Logger& {
  static Logger telemetry;
  return telemetry;
}

void Logger::registerSink(std::unique_ptr<ILogSink> sink) {
  // Add the custom log sink
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  telemetry.sinks_.emplace_back(std::move(sink));
}

void Logger::log(const LogEntry& log_entry) {
  auto& telemetry = instance();
  telemetry.log_entries_consumer_.queue().forcePush(log_entry);
}

void Logger::processLogEntries(const LogEntry& entry) {
  const std::lock_guard lock(sink_mutex_);
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

LogEntry::LogEntry(Level level_in, std::string&& module_in, std::string&& message_in,
                   std::source_location location_in)
  : level{ level_in }
  , module{ std::move(module_in) }
  , message{ std::move(message_in) }
  , location{ location_in }
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
  ss << " module=" << std::quoted(log.module);
  ss << " message=" << std::quoted(log.message);

  for (const Field<std::string>& field : log.fields) {
    ss << " " << field.key << "=" << field.val;
  }

  return ss.str();
}

}  // namespace heph::telemetry
