//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <exception>
#include <thread>

#include <exec/async_scope.hpp>
#include <fmt/chrono.h>
#include <gtest/gtest.h>
#include <hephaestus/concurrency/timer.h>
#include <hephaestus/utils/exception.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"

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
                  heph::panic("testing");
                }) |
                stdexec::upon_error([&called](std::exception_ptr eptr) {
                  try {
                    std::rethrow_exception(std::move(eptr));
                  } catch (heph::Panic&) {
                  }
                  called = true;
                });
  scope.spawn(sender);
  context.run();
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
        std::scoped_lock l{ mtx };
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
    stdexec::sync_wait(stdexec::schedule(context_ptr->scheduler()) | stdexec::then([&] {
                         ++completions;
                         EXPECT_NE(std::this_thread::get_id(), submit_thread_id);
                       }));
  }

  context_ptr->requestStop();
  runner.join();
  EXPECT_EQ(completions, NUM_TASKS);
}
TEST(ContextTests, scheduleAfter) {
  Context context{ {} };
  exec::async_scope scope;
  std::vector<int> call_sequence;
  std::vector<int> call_sequence_ref{ 2, 1 };
  std::size_t called{ 0 };
  auto delay_time = std::chrono::milliseconds(10);
  auto begin = std::chrono::steady_clock::now();
  {
    auto sender = context.scheduler().scheduleAfter(delay_time * 2) |
                  stdexec::then([&context, &called, &call_sequence] {
                    ++called;
                    context.requestStop();
                    call_sequence.push_back(1);
                  });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(delay_time) | stdexec::then([&called, &call_sequence] {
                    ++called;
                    call_sequence.push_back(2);
                  });
    scope.spawn(sender);
  }
  context.run();
  auto end = std::chrono::steady_clock::now();
  EXPECT_EQ(called, 2);
  EXPECT_EQ(call_sequence, call_sequence_ref);
  EXPECT_GE(end - begin, delay_time * 2);
  EXPECT_GE(context.elapsed(), delay_time * 2);
}

TEST(ContextTests, scheduleAfterStopWaiting) {
  Context context{ {} };
  exec::async_scope scope;
  std::size_t called{ 0 };
  auto delay_time = std::chrono::milliseconds(50);
  auto begin = std::chrono::steady_clock::now();
  {
    auto sender = context.scheduler().scheduleAfter(delay_time * 2) | stdexec::then([&called] { ++called; });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(delay_time) | stdexec::then([&called, &context] {
                    ++called;
                    context.requestStop();
                  });
    scope.spawn(sender);
  }
  context.run();
  auto end = std::chrono::steady_clock::now();
  auto duration = end - begin;
  if (duration < delay_time * 2) {
    EXPECT_EQ(called, 1);
  }
  EXPECT_GE(duration, delay_time);
}

TEST(ContextTests, scheduleAfterSimulated) {
  Context context{ { .io_ring_config = {}, .timer_options{ ClockMode::SIMULATED } } };
  exec::async_scope scope;
  std::vector<int> call_sequence;
  std::vector<int> call_sequence_ref{ 2, 1 };
  std::size_t called{ 0 };
  auto delay_time = std::chrono::minutes(1);
  auto begin = std::chrono::system_clock::now();
  {
    auto sender = context.scheduler().scheduleAfter(delay_time * 2) |
                  stdexec::then([&context, &called, &call_sequence] {
                    ++called;
                    context.requestStop();
                    call_sequence.push_back(1);
                  });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(delay_time) | stdexec::then([&called, &call_sequence] {
                    ++called;
                    call_sequence.push_back(2);
                  });
    scope.spawn(sender);
  }
  context.run();
  auto end = std::chrono::system_clock::now();
  EXPECT_EQ(called, 2);
  EXPECT_EQ(call_sequence, call_sequence_ref);
  EXPECT_LE(end - begin, delay_time);
  EXPECT_GE(context.elapsed(), delay_time * 2);
}

TEST(ContextTests, scheduleAfterStopWaitingSimulated) {
  Context context{ { .io_ring_config = {}, .timer_options{ ClockMode::SIMULATED } } };
  exec::async_scope scope;
  std::size_t called{ 0 };
  auto delay_time = std::chrono::minutes(1);
  {
    auto sender = context.scheduler().scheduleAfter(delay_time * 2) | stdexec::then([&called] { ++called; });
    scope.spawn(sender);
  }
  {
    auto sender = context.scheduler().scheduleAfter(delay_time) | stdexec::then([&called, &context] {
                    ++called;
                    context.requestStop();
                  });
    scope.spawn(sender);
  }
  auto begin = std::chrono::steady_clock::now();
  context.run();
  auto end = std::chrono::steady_clock::now();
  EXPECT_EQ(called, 1);
  EXPECT_LE(end - begin, delay_time);
  EXPECT_GE(context.elapsed(), delay_time);
  EXPECT_LT(context.elapsed(), delay_time * 2);
}

}  // namespace heph::concurrency::tests
