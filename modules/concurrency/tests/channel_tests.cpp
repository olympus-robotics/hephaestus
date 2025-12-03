//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <set>
#include <thread>

#include <absl/synchronization/mutex.h>
#include <exec/async_scope.hpp>
#include <exec/when_any.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/channel.h"
// NOLINTBEGIN(bugprone-unchecked-optional-access)
namespace heph::concurrency {
TEST(Channel, SendRecv) {
  Channel<int, 2> channel;

  {
    auto [value] = *stdexec::sync_wait(stdexec::when_all(channel.getValue(), channel.setValue(1)));

    EXPECT_EQ(value, 1);
  }
  {
    auto [value] = *stdexec::sync_wait(stdexec::when_all(channel.setValue(1), channel.getValue()));

    EXPECT_EQ(value, 1);
  }
}

TEST(Channel, SendCancel) {
  Channel<int, 1> channel;
  // set once to make channel block later on
  stdexec::sync_wait(channel.setValue(0));

  auto [res] =
      *stdexec::sync_wait(exec::when_any(stdexec::just(true), channel.setValue(0) | stdexec::then([]() {
                                                                EXPECT_TRUE(false);
                                                                return false;
                                                              })));

  EXPECT_TRUE(res);
}

TEST(Channel, GetCancel) {
  Channel<int, 2> channel;

  auto [res] = *stdexec::sync_wait(exec::when_any(
      stdexec::just(true), channel.getValue() | stdexec::then([](auto /*unused*/) { return false; })));

  EXPECT_TRUE(res);
}

TEST(Channel, SendRecvOrder) {
  Channel<int, 4> channel;

  stdexec::sync_wait(channel.setValue(0));
  stdexec::sync_wait(channel.setValue(1));
  stdexec::sync_wait(channel.setValue(2));
  stdexec::sync_wait(channel.setValue(3));

  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 0);
  }
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 1);
  }
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 2);
  }
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 3);
  }
  stdexec::sync_wait(channel.setValue(1));
  stdexec::sync_wait(channel.setValue(2));
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 1);
  }
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 2);
  }
}

TEST(Channel, SendRecvParallel) {
  Channel<std::size_t, 4> channel;
  static constexpr std::size_t NUMBER_OF_ITERATIONS = 10000;

  std::thread producer{ [&]() {
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      stdexec::sync_wait(channel.setValue(i));
    }
  } };
  std::thread consumer{ [&]() {
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      auto [res] = *stdexec::sync_wait(channel.getValue());
      EXPECT_EQ(res, i);
    }
  } };

  producer.join();
  consumer.join();
}

TEST(Channel, SendRecvParallelScope) {
  Channel<std::size_t, 4> channel;
  static constexpr std::size_t NUMBER_OF_ITERATIONS = 10000;

  std::thread producer{ [&]() {
    exec::async_scope scope;
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      scope.spawn(channel.setValue(i));
    }
    stdexec::sync_wait(scope.on_empty());
  } };

  std::thread consumer{ [&]() {
    exec::async_scope scope;
    absl::Mutex mutex;
    std::set<int> received_values;
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      scope.spawn(channel.getValue() | stdexec::then([&received_values, &mutex](int value) {
                    // The continuation might run on the produce thread...
                    const absl::MutexLock lock{ &mutex };
                    auto [_, inserted] = received_values.insert(value);
                    EXPECT_TRUE(inserted);
                  }));
    }
    stdexec::sync_wait(scope.on_empty());
  } };

  producer.join();
  consumer.join();
}

TEST(Channel, SendRecvParallelScopeStop) {
  Channel<std::size_t, 4> channel;
  static constexpr std::size_t NUMBER_OF_ITERATIONS = 10000;

  std::thread producer{ [&]() {
    exec::async_scope scope;
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      scope.spawn(channel.setValue(i));
    }
    // Just give a little leeway to have some executed...
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    scope.request_stop();
    stdexec::sync_wait(scope.on_empty());
  } };

  std::thread consumer{ [&]() {
    exec::async_scope scope;
    absl::Mutex mutex;
    std::set<int> received_values;
    std::atomic<std::size_t> stopped_count;
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      scope.spawn(channel.getValue() | stdexec::then([&received_values, &mutex](int value) {
                    // The continuation might run on the produce thread...
                    const absl::MutexLock lock{ &mutex };
                    auto [_, inserted] = received_values.insert(value);
                    EXPECT_TRUE(inserted);
                  }) |
                  stdexec::upon_stopped([&stopped_count]() { ++stopped_count; }));
    }
    // Just give a little leeway to have some executed...
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    scope.request_stop();
    stdexec::sync_wait(scope.on_empty());
    EXPECT_EQ(received_values.size() + stopped_count.load(), NUMBER_OF_ITERATIONS);
  } };

  producer.join();
  consumer.join();
}

}  // namespace heph::concurrency
// NOLINTEND(bugprone-unchecked-optional-access)
