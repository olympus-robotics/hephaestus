//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/log.h"

#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/telemetry/log_sink.h"

namespace {
class Logger final {
public:
  explicit Logger();
  ~Logger();

  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  auto operator=(const Logger&) -> Logger& = delete;
  auto operator=(Logger&&) -> Logger& = delete;

  static void registerSink(std::unique_ptr<heph::telemetry::ILogSink> sink);

  static void log(heph::telemetry::LogEntry&& log_entry);

private:
  [[nodiscard]] static auto instance() -> Logger&;

  /// @brief Do the actual logging. Take in LogEntry and send it to all sinks
  /// @param LogEntry, class that takes in the structured logs and formats them.
  /// @return void
  void processLogEntries(const heph::telemetry::LogEntry& entry);

private:
  absl::Mutex sink_mutex_;
  // This is used as registry for the log sinks
  std::vector<std::unique_ptr<heph::telemetry::ILogSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  heph::concurrency::MessageQueueConsumer<heph::telemetry::LogEntry> log_entries_consumer_;
};

Logger::Logger()
  : log_entries_consumer_([this](const heph::telemetry::LogEntry& entry) { processLogEntries(entry); },
                          std::nullopt) {
  log_entries_consumer_.start();
}

Logger::~Logger() {
  try {
    log_entries_consumer_.stop().get();
  } catch (const std::exception& ex) {
    std::cerr << "While emptying message consumer, exception happened: " << ex.what() << "\n";
  }
}

auto Logger::instance() -> Logger& {
  static Logger telemetry;
  return telemetry;
}

void Logger::registerSink(std::unique_ptr<heph::telemetry::ILogSink> sink) {
  // Add the custom log sink
  auto& telemetry = instance();
  const absl::MutexLock lock{ &telemetry.sink_mutex_ };
  telemetry.sinks_.emplace_back(std::move(sink));
}

void Logger::log(heph::telemetry::LogEntry&& log_entry) {
  auto& telemetry = instance();
  telemetry.log_entries_consumer_.queue().forcePush(std::move(log_entry));
}

void Logger::processLogEntries(const heph::telemetry::LogEntry& entry) {
  const absl::MutexLock lock{ &sink_mutex_ };
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}
}  // namespace

void heph::telemetry::internal::log(heph::telemetry::LogEntry&& log_entry) {
  Logger::log(std::move(log_entry));
}

void heph::telemetry::registerLogSink(std::unique_ptr<heph::telemetry::ILogSink> sink) {
  Logger::registerSink(std::move(sink));
}
