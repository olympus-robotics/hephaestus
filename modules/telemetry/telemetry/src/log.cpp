//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/log.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <future>
#include <memory>
#include <new>  // std::bad_alloc
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>
#include <fmt/base.h>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/stack_trace.h"

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

  static void registerSink(std::unique_ptr<ILogSink> sink) noexcept;

  static void log(LogEntry&& log_entry) noexcept;
  static void flush() noexcept;

private:
  [[nodiscard]] static auto instance() noexcept -> Logger&;

  /// @brief Do the actual logging. Take in LogEntry and send it to all sinks
  /// @param LogEntry, class that takes in the structured logs and formats them.
  /// @return void
  void processEntry(const LogEntry& entry) noexcept;

  /// @brief Empty the queue so that remaining messages get processed
  void emptyQueue() noexcept;

private:
  absl::Mutex sink_mutex_;
  // This is used as registry for the log sinks
  std::vector<std::unique_ptr<ILogSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  heph::containers::BlockingQueue<LogEntry> entries_;
  std::future<void> message_process_future_;
  std::atomic<std::size_t> entries_in_flight_{ 0 };
};

Logger::Logger() : entries_{ std::nullopt } {
  message_process_future_ = std::async(std::launch::async, [this]() {
    while (true) {
      auto message = entries_.waitAndPop();
      if (!message.has_value()) {
        break;
      }

      processEntry(message.value());
      --entries_in_flight_;
    }
    emptyQueue();
  });
}

Logger::~Logger() {
  flush();

  try {
    entries_.stop();
    message_process_future_.get();
  } catch (const std::exception& ex) {
    fmt::println(stderr, "While emptying log queue, exception happened: {}", ex.what());
  }
}

auto Logger::instance() noexcept -> Logger& {
  static Logger telemetry;
  return telemetry;
}

void Logger::registerSink(std::unique_ptr<ILogSink> sink) noexcept {
  // Add the custom log sink
  auto& telemetry = instance();
  const absl::MutexLock lock{ &telemetry.sink_mutex_ };
  try {
    telemetry.sinks_.emplace_back(std::move(sink));
  } catch (const std::bad_alloc& ex) {
    fmt::println(stderr, "While registering log sink, bad allocation happened: {}", ex.what());
  }
}

void Logger::log(LogEntry&& log_entry) noexcept {
  auto& telemetry = instance();
  if (log_entry.level == LogLevel::FATAL) {
    log_entry.stack_trace = heph::utils::StackTrace::print();
  }
  try {
    ++telemetry.entries_in_flight_;
    telemetry.entries_.forcePush(std::move(log_entry));
  } catch (const std::bad_alloc& ex) {
    fmt::println(stderr, "While pushing log entry, bad allocation happened: {}", ex.what());
  }
}

void Logger::flush() noexcept {
  auto& telemetry = instance();
  while (telemetry.entries_in_flight_.load() > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void Logger::processEntry(const LogEntry& entry) noexcept {
  const absl::MutexLock lock{ &sink_mutex_ };
  if (sinks_.empty()) {
    fmt::println(stderr, "########################################################\n"
                         "REGISTER A LOG SINK TO SEE THE MESSAGES\n"
                         "########################################################\n");
  }
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

void Logger::emptyQueue() noexcept {
  while (!entries_.empty()) {
    auto message = entries_.tryPop();
    if (!message.has_value()) {
      return;
    }

    processEntry(message.value());
  }
}
}  // namespace

void internal::log(LogEntry&& log_entry) noexcept {
  Logger::log(std::move(log_entry));
}

void registerLogSink(std::unique_ptr<ILogSink> sink) noexcept {
  Logger::registerSink(std::move(sink));
}

void flushLogEntries() {
  Logger::flush();
}
}  // namespace heph::telemetry
