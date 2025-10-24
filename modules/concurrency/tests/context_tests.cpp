//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/when_any.hpp>
#include <gtest/gtest.h>
#include <hephaestus/concurrency/io_ring/timer.h>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/context_scheduler.h"

namespace heph::concurrency {}

namespace heph::concurrency::tests {
TEST(ContextTests, SchedulerBasics) {
  EXPECT_TRUE(stdexec::scheduler<Context::Scheduler>);

  Context context{ {} };
  exec::async_scope scope;
  bool called{ false };
  auto sender = stdexec::schedule(context.scheduler()) | stdexec::then([&context, &called] {
                  called = true;
                  context.requestStop();
                });
  scope.spawn(sender);
  context.run();
  stdexec::sync_wait(scope.on_empty());
  EXPECT_TRUE(called);

  EXPECT_TRUE(stdexec::sender<decltype(sender)>);
  EXPECT_TRUE((stdexec::sender_in<decltype(sender), stdexec::__sync_wait::__env>));
}

TEST(ContextTests, scheduleException) {
  EXPECT_TRUE(stdexec::scheduler<Context::Scheduler>);

  Context context{ {} };
  exec::async_scope scope;
  bool called{ false };
  auto sender = stdexec::schedule(context.scheduler()) | stdexec::then([&context] {
                  context.requestStop();
                  throw std::runtime_error("test exception");
                }) |
                stdexec::upon_error([&called](const std::exception_ptr& eptr) {
                  try {
                    std::rethrow_exception(eptr);
                  } catch (std::runtime_error&) {  // NOLINT(bugprone-empty-catch)
                  }
                  called = true;
                });
  scope.spawn(sender);
  context.run();
  stdexec::sync_wait(scope.on_empty());
  EXPECT_TRUE(called);
}

TEST(ContextTests, scheduleConcurrent) {
  std::mutex mtx;
  std::condition_variable cv;
  Context* context_ptr{ nullptr };

  std::thread runner{ [&context_ptr, &mtx, &cv] {
    Context context{ {} };
    context.run([&] {
      {
        const std::scoped_lock l{ mtx };
        context_ptr = &context;
      }
      cv.notify_all();
    });
  } };
  {
    std::unique_lock l{ mtx };
    cv.wait(l, [&context_ptr] { return context_ptr != nullptr; });
  }

  std::size_t completions{ 0 };
  static constexpr std::size_t NUM_TASKS = 10000;
  auto submit_thread_id = std::this_thread::get_id();
  for (std::size_t i = 0; i != NUM_TASKS; ++i) {
    stdexec::sync_wait(stdexec::schedule(context_ptr->scheduler()) | stdexec::then([&] {
                         ++completions;
                         EXPECT_NE(std::this_thread::get_id(), submit_thread_id);
                       }));
  }

  context_ptr->requestStop();
  runner.join();
  EXPECT_EQ(completions, NUM_TASKS);
}

TEST(ContextTests, scheduleConcurrentScope) {
  std::mutex mtx;
  std::condition_variable cv;
  Context* context_ptr{ nullptr };

  std::thread runner{ [&context_ptr, &mtx, &cv] {
    Context context{ {} };
    context.run([&] {
      {
        const std::scoped_lock l{ mtx };
        context_ptr = &context;
      }
      cv.notify_all();
    });
  } };
  {
    std::unique_lock l{ mtx };
    cv.wait(l, [&context_ptr] { return context_ptr != nullptr; });
  }

  std::size_t completions{ 0 };
  exec::async_scope scope;
  static constexpr std::size_t NUM_TASKS = 10000;
  auto submit_thread_id = std::this_thread::get_id();
  for (std::size_t i = 0; i != NUM_TASKS; ++i) {
    scope.spawn(stdexec::schedule(context_ptr->scheduler()) | stdexec::then([&] {
                  ++completions;
                  EXPECT_NE(std::this_thread::get_id(), submit_thread_id);
                }));
  }

  scope.request_stop();
  context_ptr->requestStop();
  runner.join();
  EXPECT_GE(completions, 1);
  EXPECT_LE(completions, NUM_TASKS);
}

TEST(ContextTests, scheduleAfter) {
  Context context{ {} };
  exec::async_scope scope;
  std::vector<int> call_sequence;
  const std::vector<int> call_sequence_ref{ 2, 1 };
  std::size_t called{ 0 };
  static constexpr auto DELAY_TIME = std::chrono::milliseconds(10);
  auto begin = std::chrono::steady_clock::now();
  {
    auto sender = context.scheduler().scheduleAfter(DELAY_TIME * 2) |
                  stdexec::then([&context, &called, &call_sequence] {
                    ++called;
                    context.requestStop();
                    call_sequence.push_back(1);
                  });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(DELAY_TIME) | stdexec::then([&called, &call_sequence] {
                    ++called;
                    call_sequence.push_back(2);
                  });
    scope.spawn(sender);
  }
  context.run();
  stdexec::sync_wait(scope.on_empty());
  auto end = std::chrono::steady_clock::now();
  EXPECT_EQ(called, 2);
  EXPECT_EQ(call_sequence, call_sequence_ref);
  EXPECT_GE(end - begin, DELAY_TIME * 2);
  EXPECT_GE(context.elapsed(), DELAY_TIME * 2);
}

TEST(ContextTests, scheduleAfterStopWaiting) {
  Context context{ {} };
  exec::async_scope scope;
  std::size_t called{ 0 };
  static constexpr auto DELAY_TIME = std::chrono::milliseconds(50);
  auto begin = std::chrono::steady_clock::now();
  {
    auto sender = context.scheduler().scheduleAfter(DELAY_TIME * 2) | stdexec::then([&called] { ++called; });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(DELAY_TIME) | stdexec::then([&called, &context, &scope] {
                    ++called;
                    scope.request_stop();
                    context.requestStop();
                  });
    scope.spawn(sender);
  }
  context.run();
  stdexec::sync_wait(scope.on_empty());
  auto end = std::chrono::steady_clock::now();
  auto duration = end - begin;
  if (duration < DELAY_TIME * 2) {
    EXPECT_EQ(called, 1);
  }
  EXPECT_GE(duration, DELAY_TIME);
}

TEST(ContextTests, scheduleAfterStopWaitingAny) {
  Context context{ {} };
  exec::async_scope scope;
  static constexpr auto DELAY_TIME = std::chrono::minutes(5);

  auto begin = std::chrono::steady_clock::now();
  scope.spawn(exec::when_any(context.scheduler().scheduleAfter(DELAY_TIME), context.scheduler().schedule()) |
              stdexec::then([&] { context.requestStop(); }));

  context.run();
  stdexec::sync_wait(scope.on_empty());
  auto end = std::chrono::steady_clock::now();
  auto duration = end - begin;
  EXPECT_LT(duration, DELAY_TIME);
}

TEST(ContextTests, scheduleAfterSimulated) {
  Context context{ { .io_ring_config = {}, .timer_options{ io_ring::ClockMode::SIMULATED } } };
  exec::async_scope scope;
  std::vector<int> call_sequence;
  const std::vector<int> call_sequence_ref{ 2, 1 };
  std::size_t called{ 0 };
  static constexpr auto DELAY_TIME = std::chrono::minutes(1);
  auto begin = std::chrono::system_clock::now();
  {
    auto sender = context.scheduler().scheduleAfter(DELAY_TIME * 2) |
                  stdexec::then([&context, &called, &call_sequence, &scope] {
                    ++called;
                    scope.request_stop();
                    context.requestStop();
                    call_sequence.push_back(1);
                  });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(DELAY_TIME) | stdexec::then([&called, &call_sequence] {
                    ++called;
                    call_sequence.push_back(2);
                  });
    scope.spawn(sender);
  }
  context.run();
  stdexec::sync_wait(scope.on_empty());
  auto end = std::chrono::system_clock::now();
  EXPECT_EQ(called, 2);
  EXPECT_EQ(call_sequence, call_sequence_ref);
  EXPECT_LE(end - begin, DELAY_TIME);
  EXPECT_GE(context.elapsed(), DELAY_TIME * 2);
}

TEST(ContextTests, scheduleAfterStopWaitingSimulated) {
  Context context{ { .io_ring_config = {}, .timer_options{ io_ring::ClockMode::SIMULATED } } };
  exec::async_scope scope;
  std::size_t called{ 0 };
  auto delay_time = std::chrono::minutes(1);
  {
    auto sender = context.scheduler().scheduleAfter(delay_time * 2) | stdexec::then([&called] { ++called; });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(delay_time) | stdexec::then([&called, &context, &scope] {
                    ++called;
                    scope.request_stop();
                    context.requestStop();
                  });
    scope.spawn(sender);
  }
  auto begin = std::chrono::steady_clock::now();
  context.run();
  auto end = std::chrono::steady_clock::now();
  stdexec::sync_wait(scope.on_empty());
  EXPECT_EQ(called, 1);
  EXPECT_LE(end - begin, delay_time);
  EXPECT_GE(context.elapsed(), delay_time);
  EXPECT_LE(context.elapsed(), delay_time * 2);
}

}  // namespace heph::concurrency::tests
