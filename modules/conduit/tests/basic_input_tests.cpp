//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <utility>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/conditional.h"
#include "hephaestus/conduit/generator.h"
#include "hephaestus/conduit/periodic.h"
#include "hephaestus/conduit/scheduler.h"

namespace heph::conduit {

struct JustInput : BasicInput {
  JustInput() noexcept : BasicInput("just") {
  }
  auto doTrigger(SchedulerT /*scheduler*/) -> SenderT final {
    return stdexec::just(true);
  }

  void handleCompleted() final {
  }
};

TEST(BasicInput, JustInput) {
  auto start_time = ClockT::now();
  JustInput input;
  EXPECT_EQ(input.name(), "just");

  heph::concurrency::Context context{ {} };

  auto trigger = input.trigger(context.scheduler());

  auto res = stdexec::sync_wait(std::move(trigger));
  EXPECT_TRUE(res.has_value());
  EXPECT_NE(input.lastTriggerTime(), ClockT::time_point{});
  EXPECT_LE(input.lastTriggerTime(), ClockT::now());
  EXPECT_GE(input.lastTriggerTime(), start_time);
}

struct JustStoppedInput : BasicInput {
  bool completed = false;
  JustStoppedInput() noexcept : BasicInput("just_stopped") {
  }
  auto doTrigger(SchedulerT /*scheduler*/) -> SenderT final {
    return stdexec::just_stopped();
  }
  void handleCompleted() final {
    completed = true;
  }
};

TEST(BasicInput, JustStoppedInput) {
  JustStoppedInput input;
  EXPECT_EQ(input.name(), "just_stopped");

  heph::concurrency::Context context{ {} };

  auto trigger = input.trigger(context.scheduler());

  auto res = stdexec::sync_wait(std::move(trigger));
  EXPECT_FALSE(res.has_value());
  EXPECT_FALSE(input.completed);
  EXPECT_EQ(input.lastTriggerTime(), ClockT::time_point{});
}

struct JustCoroutineInput : BasicInput {
  JustCoroutineInput() noexcept : BasicInput("just_coroutine") {
  }
  auto doTrigger(SchedulerT /*scheduler*/) -> SenderT final {
    return doTriggerImpl();
  }
  static auto doTriggerImpl() -> exec::task<bool> {
    co_return true;
  }
  void handleCompleted() final {
  }
};

TEST(BasicInput, JustCoroutineInput) {
  auto start_time = ClockT::now();
  JustCoroutineInput input;
  EXPECT_EQ(input.name(), "just_coroutine");

  heph::concurrency::Context context{ {} };

  auto trigger = input.trigger(context.scheduler());

  auto res = stdexec::sync_wait(std::move(trigger));
  EXPECT_TRUE(res.has_value());
  EXPECT_NE(input.lastTriggerTime(), ClockT::time_point{});
  EXPECT_LE(input.lastTriggerTime(), ClockT::now());
  EXPECT_GE(input.lastTriggerTime(), start_time);
}

// TODO: (heller) enable again once timer got fully reworked
TEST(BasicInput, DISABLED_PeriodicCancelled) {
  Periodic periodic;
  exec::async_scope scope;

  periodic.setPeriodDuration(std::chrono::hours{ 1 });

  auto start_time = ClockT::now();
  {
    concurrency::Context context{ {} };
    auto stop = stdexec::then([&]() { context.requestStop(); });
    bool triggered = false;
    scope.spawn(
        // The first trigger always succeeds since it is the first iteration
        periodic.trigger(context.scheduler()) |
        // The second trigger is now supposed to be canceled
        stdexec::let_value([&]() {
          start_time = periodic.lastTriggerTime();
          return periodic.trigger(context.scheduler()) | stdexec::then([&]() { triggered = true; });
        }));
    scope.spawn(context.scheduler().scheduleAfter(std::chrono::milliseconds{ 1 }) | stop);

    context.run();
    EXPECT_FALSE(triggered);
    EXPECT_GE(periodic.lastTriggerTime(), start_time);
  }
  {
    concurrency::Context context{ {} };
    auto stop = stdexec::then([&]() { context.requestStop(); });
    bool triggered = false;
    scope.spawn(exec::when_any(context.scheduler().scheduleAfter(std::chrono::milliseconds{ 1 }),
                               // The first trigger always succeeds since it is the first iteration
                               periodic.trigger(context.scheduler()) |
                                   // The second trigger is now supposed to be canceled
                                   stdexec::let_value([&]() {
                                     start_time = periodic.lastTriggerTime();
                                     return periodic.trigger(context.scheduler()) |
                                            stdexec::then([&]() { triggered = true; });
                                   })) |
                stop);

    context.run();
    scope.request_stop();
    stdexec::sync_wait(scope.on_empty());
    EXPECT_FALSE(triggered);
    EXPECT_GE(periodic.lastTriggerTime(), start_time);
  }
}

TEST(BasicInput, PeriodicSuccess) {
  auto start_time = ClockT::now();
  Periodic periodic;
  concurrency::Context context{ {} };
  exec::async_scope scope;

  static constexpr std::chrono::milliseconds DURATION{ 1 };
  periodic.setPeriodDuration(DURATION);

  std::size_t triggered = 0;
  scope.spawn(periodic.trigger(context.scheduler()) | stdexec::let_value([&]() {
                ++triggered;
                return periodic.trigger(context.scheduler()) | stdexec::then([&]() {
                         ++triggered;
                         context.requestStop();
                       });
              }));

  context.run();
  EXPECT_NE(periodic.lastTriggerTime(), ClockT::time_point{});
  EXPECT_LE(periodic.lastTriggerTime(), ClockT::now());
  EXPECT_GE(periodic.lastTriggerTime(), start_time);
  EXPECT_GE(periodic.lastTriggerTime() - start_time, DURATION);
  EXPECT_EQ(triggered, 2);
}

TEST(BasicInput, ConditionalCancelled) {
  Conditional conditional;
  conditional.disable();
  concurrency::Context context{ {} };
  exec::async_scope scope;

  bool triggered = false;

  scope.spawn(exec::when_any(stdexec::just(), conditional.trigger(context.scheduler()) |
                                                  stdexec::then([&]() { triggered = true; })));
  EXPECT_FALSE(triggered);
  EXPECT_EQ(conditional.lastTriggerTime(), ClockT::time_point{});
}

TEST(BasicInput, ConditionalTrigger) {
  Conditional conditional;
  concurrency::Context context{ {} };

  static constexpr std::size_t NUMBER_OF_ITERATIONS = 100;
  for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
    conditional.enable();
    auto res = stdexec::sync_wait(conditional.trigger(context.scheduler()));
    EXPECT_TRUE(res.has_value());

    conditional.disable();
    bool triggered = false;

    res = stdexec::sync_wait(exec::when_any(stdexec::just(), conditional.trigger(context.scheduler()) |
                                                                 stdexec::then([&]() { triggered = true; })));
    EXPECT_TRUE(res.has_value());
    EXPECT_FALSE(triggered);
  }
}

TEST(BasicInput, ConditionalTriggerConcurrent) {
  Conditional conditional;
  concurrency::Context context{ {} };
  exec::async_scope scope;

  std::size_t num_triggered{ 0 };

  static constexpr std::size_t NUMBER_OF_ITERATIONS = 100;

  scope.spawn(heph::concurrency::repeatUntil([&]() {
                auto scheduler = context.scheduler();
                return stdexec::continues_on(conditional.trigger(scheduler), scheduler) |
                       stdexec::then([&]() {
                         ++num_triggered;
                         return num_triggered == NUMBER_OF_ITERATIONS;
                       });
              }) |
              stdexec::then([&]() {
                scope.request_stop();
                context.requestStop();
              }));

  scope.spawn(heph::concurrency::repeatUntil([&]() {
    return stdexec::schedule(context.scheduler()) | stdexec::let_value([&]() {
             conditional.enable();
             return context.scheduler().scheduleAfter(std::chrono::microseconds{ 1 }) | stdexec::then([&]() {
                      conditional.disable();
                      return false;
                    });
           });
  }));
  context.run();
  scope.request_stop();
  stdexec::sync_wait(scope.on_empty());
  EXPECT_EQ(num_triggered, NUMBER_OF_ITERATIONS);
}

TEST(BasicInput, ConditionalTriggerParallel) {
  Conditional conditional;
  concurrency::Context context{ {} };
  exec::async_scope scope;

  std::atomic_bool done{ false };
  std::size_t num_triggered{ 0 };

  static constexpr std::size_t NUMBER_OF_ITERATIONS = 100;

  auto scheduler = context.scheduler();
  scope.spawn(heph::concurrency::repeatUntil([&]() {
                return stdexec::continues_on(conditional.trigger(scheduler), scheduler) |
                       stdexec::then([&]() {
                         ++num_triggered;
                         return num_triggered == NUMBER_OF_ITERATIONS;
                       });
              }) |
              stdexec::then([&]() {
                scope.request_stop();
                context.requestStop();
                done.store(true, std::memory_order_release);
              }));

  std::thread trigger([&]() {
    while (!done.load(std::memory_order_acquire)) {
      conditional.enable();
      std::this_thread::sleep_for(std::chrono::microseconds{ 1 });
      conditional.disable();
    }
  });
  context.run();
  trigger.join();
  scope.request_stop();
  stdexec::sync_wait(scope.on_empty());
  EXPECT_EQ(num_triggered, NUMBER_OF_ITERATIONS);
}

TEST(BasicInput, Generator) {
  Generator<int> generator{ "generator" };
  concurrency::Context context{ {} };

  static constexpr int VALUE = 4711;
  auto test = [&] {
    auto res = stdexec::sync_wait(generator.trigger(context.scheduler()));
    EXPECT_TRUE(res.has_value());
    EXPECT_NE(generator.lastTriggerTime(), ClockT::time_point{});
    EXPECT_TRUE(generator.hasValue());
    EXPECT_EQ(generator.value(), VALUE);
  };

  generator.setGenerator([]() { return VALUE; });
  test();
  generator.setGenerator([]() { return stdexec::just(VALUE); });
  test();
  generator.setGenerator([]() -> exec::task<int> { co_return VALUE; });
  test();
}
}  // namespace heph::conduit
