//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/log/log.h"

#include <algorithm>
#include <atomic>
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
#include "hephaestus/telemetry/log/log_sink.h"

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

  auto registerSink(std::unique_ptr<ILogSink>&& sink) noexcept -> ILogSink&;
  [[nodiscard]] auto removeSink(ILogSink& sink) noexcept -> bool;
  void removeAllSinks() noexcept;

  void log(LogEntry&& log_entry) noexcept;
  void flush() noexcept;

private:
  void runThread();

  /// @brief Do the actual logging. Take in LogEntry and send it to all sinks
  /// @param LogEntry, class that takes in the structured logs and formats them.
  /// @return void
  void processEntry(const LogEntry& entry) noexcept;

private:
  absl::Mutex sink_mutex_;
  // This is used as registry for the log sinks
  std::vector<std::unique_ptr<ILogSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  static constexpr auto MAX_LOG_QUEUE_SIZE = 100;
  heph::containers::BlockingQueue<LogEntry> entries_;
  std::future<void> message_process_future_;
  std::atomic<std::size_t> entries_in_flight_{ 0 };
};

void Logger::runThread() {
  while (true) {
    auto message = entries_.waitAndPop();

    if (!message.has_value()) {
      break;
    }

    processEntry(message.value());
    entries_in_flight_.fetch_sub(1, std::memory_order::release);
  }

  // Drain queue before exiting
  while (!entries_.empty()) {
    auto message = entries_.tryPop();

    if (!message.has_value()) {
      return;
    }

    processEntry(message.value());
    entries_in_flight_.fetch_sub(1, std::memory_order::release);
  }
}

Logger::Logger()
  : entries_{ MAX_LOG_QUEUE_SIZE }
  , message_process_future_{ std::async(std::launch::async, [this] { runThread(); }) } {
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

auto Logger::registerSink(std::unique_ptr<ILogSink>&& sink) noexcept -> ILogSink& {
  const absl::MutexLock lock{ &sink_mutex_ };
  auto& result = *sink;

  try {
    sinks_.emplace_back(std::move(sink));
  } catch (const std::bad_alloc& ex) {
    fmt::println(stderr, "While registering log sink, bad allocation happened: {}", ex.what());
  }

  return result;
}

auto Logger::removeSink(ILogSink& sink) noexcept -> bool {
  const absl::MutexLock lock{ &sink_mutex_ };

  auto it = std::ranges::find_if(sinks_, [&](const auto& uptr) { return uptr.get() == &sink; });

  if (it == sinks_.end()) {
    return false;
  }

  sinks_.erase(it);
  return true;
}

void Logger::removeAllSinks() noexcept {
  const absl::MutexLock lock{ &sink_mutex_ };
  sinks_.clear();
}

void Logger::log(LogEntry&& log_entry) noexcept {
  entries_in_flight_.fetch_add(1, std::memory_order::acquire);

  try {
    const auto dropped = entries_.forcePush(std::move(log_entry));

    if (dropped.has_value()) {
      entries_in_flight_.fetch_sub(1, std::memory_order::relaxed);

      fmt::println(stderr,
                   "[DANGER] Log entry dropped as queue is full. This shouldn't happen! Consider extending "
                   "the queue or improving sink processes. Log message is:\n\t{}",
                   *dropped);
    }
  } catch (const std::bad_alloc& ex) {
    entries_in_flight_.fetch_sub(1, std::memory_order::relaxed);
    fmt::println(stderr, "While pushing log entry, bad allocation happened: {}", ex.what());
  }
}

void Logger::flush() noexcept {
  entries_.waitForEmpty();

  while (entries_in_flight_.load(std::memory_order::acquire) > 0) {
    std::this_thread::yield();
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

[[nodiscard]] auto getLoggerInstance() noexcept -> Logger& {
  static Logger logger;
  return logger;
}

}  // namespace

void internal::log(LogEntry&& log_entry) noexcept {
  getLoggerInstance().log(std::move(log_entry));
}

auto registerLogSink(std::unique_ptr<ILogSink> sink) noexcept -> ILogSink& {
  return getLoggerInstance().registerSink(std::move(sink));
}

auto removeLogSink(ILogSink& sink) noexcept -> bool {
  return getLoggerInstance().removeSink(sink);
}

void removeAllLogSinks() noexcept {
  getLoggerInstance().removeAllSinks();
}

void flushLogEntries() {
  getLoggerInstance().flush();
}

}  // namespace heph::telemetry
