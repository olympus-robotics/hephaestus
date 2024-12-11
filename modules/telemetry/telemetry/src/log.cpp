//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/log.h"

#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry {
namespace {
class Logger final {
public:
  explicit Logger();
  ~Logger();

  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  auto operator=(const Logger&) -> Logger& = delete;
  auto operator=(Logger&&) -> Logger& = delete;

  static void registerSink(std::unique_ptr<ILogSink> sink);

  static void log(LogEntry&& log_entry);

private:
  [[nodiscard]] static auto instance() -> Logger&;

  /// @brief Do the actual logging. Take in LogEntry and send it to all sinks
  /// @param LogEntry, class that takes in the structured logs and formats them.
  /// @return void
  void processEntry(const LogEntry& entry);

  /// @brief Empty the queue so that remaining messages get processed
  void emptyQueue();

private:
  absl::Mutex sink_mutex_;
  // This is used as registry for the log sinks
  std::vector<std::unique_ptr<ILogSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  heph::containers::BlockingQueue<LogEntry> entries_;
  std::future<void> message_process_future_;
};

Logger::Logger() : entries_{ std::nullopt } {
  message_process_future_ = std::async(std::launch::async, [this]() {
    while (true) {
      auto message = entries_.waitAndPop();
      if (!message.has_value()) {
        emptyQueue();
        return;
      }

      processEntry(message.value());
    }
  });
}

Logger::~Logger() {
  try {
    entries_.stop();
    message_process_future_.get();
  } catch (const std::exception& ex) {
    std::cerr << "While emptying log queue, exception happened: " << ex.what() << "\n";
  }
}

auto Logger::instance() -> Logger& {
  static Logger telemetry;
  return telemetry;
}

void Logger::registerSink(std::unique_ptr<ILogSink> sink) {
  // Add the custom log sink
  auto& telemetry = instance();
  const absl::MutexLock lock{ &telemetry.sink_mutex_ };
  telemetry.sinks_.emplace_back(std::move(sink));
}

void Logger::log(LogEntry&& log_entry) {
  auto& telemetry = instance();
  telemetry.entries_.forcePush(std::move(log_entry));
}

void Logger::processEntry(const LogEntry& entry) {
  const absl::MutexLock lock{ &sink_mutex_ };
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

void Logger::emptyQueue() {
  while (!entries_.empty()) {
    auto message = entries_.tryPop();
    if (!message.has_value()) {
      return;
    }

    processEntry(message.value());
  }
}
}  // namespace

void internal::log(LogEntry&& log_entry) {
  Logger::log(std::move(log_entry));
}

void registerLogSink(std::unique_ptr<ILogSink> sink) {
  Logger::registerSink(std::move(sink));
}
}  // namespace heph::telemetry
