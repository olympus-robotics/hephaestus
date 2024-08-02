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
#include <numeric>
#include <optional>
#include <ostream>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <unistd.h>

#include "hephaestus/concurrency/message_queue_consumer.h"

namespace heph::telemetry {
namespace {
auto getHostname() -> std::string {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  std::array<char, 256> buffer{};

  if (gethostname(buffer.data(), buffer.size()) != 0) {
    return "---";
  }

  return { buffer.data() };
}

/** @brief Do the actual logging. Take in LogEntry and send it to all sinks
 *  @param LogEntry, class that takes in the structured logs and formats them.
 *  @return void
 */
void processLogEntries(const LogEntry& entry) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  for (const auto& sink:telemetry.sinks_){
    sink->send(entry);
  }
}
}  // namespace

auto operator<<(std::ostream& os, const Level& level) -> std::ostream& {
  switch (level) {
    case Level::Trace:
      os << "trace";
      break;
    case Level::Debug:
      os << "debug";
      break;
    case Level::Info:
      os << "info";
      break;
    case Level::Warn:
      os << "warn";
      break;
    case Level::Error:
      os << "error";
      break;
    case Level::Fatal:
      os << "fatal";
      break;
  }

  return os;
}


class Logger final {
public:
  explicit Logger();
  static void registerSink(std::unique_ptr<ILogSink> sink);

  static void log(const LogEntry& log_entry);

private:
  [[nodiscard]] static auto instance() -> Logger&;

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
  : log_entries_consumer_([](const LogEntry& entry) { processLogEntries(entry); }, std::nullopt) {
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

LogEntry::LogEntry(Level level, std::string_view message, std::source_location location)
  : level{ level }
  , message{ message }
  , location{ location }
  , thread_id{ std::this_thread::get_id() }
  , time{ LogEntry::Clock::now() }
  , hostname{ getHostname() } {
}

auto format(const LogEntry& log) -> std::string {
  std::stringstream ss;
  ss << "level=" << log.level;
  ss << " hostname=" << std::quoted(log.hostname);
  ss << " location="
     << std::quoted(fmt::format("{}:{}",
                                std::filesystem::path{ log.location.file_name() }.filename().string(),
                                log.location.line()));
  ss << " thread-id=" << log.thread_id << " message=" << std::quoted(log.message);
  ss << " time=" << fmt::format("{:%Y-%m-%dT%H:%M:%SZ}", log.time);
  return std::accumulate(log.fields.cbegin(), log.fields.cend(), ss.str(),
                         [](const std::string& accumulated, const Field<std::string>& next) {
                           return fmt::format("{} {}={}", accumulated, next.key, next.val);
                         });
}

}  // namespace heph::telemetry