//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <thread>

#include <exec/when_any.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/channel.h"
// NOLINTBEGIN(bugprone-unchecked-optional-access)
namespace heph::concurrency {
TEST(Channel, SendRecv) {
  Channel<int, 1> channel;

  EXPECT_EQ(channel.size(), 0);

  {
    auto [value] =
        *stdexec::sync_wait(stdexec::when_all(channel.getValue(), channel.setValue(1) | stdexec::then([&]() {
                                                                    // The size is zero here since the
                                                                    // getValue awaiter will be
                                                                    // immediately invoked before this
                                                                    // set completes
                                                                    EXPECT_EQ(channel.size(), 0);
                                                                  })));

    EXPECT_EQ(value, 1);
    EXPECT_EQ(channel.size(), 0);
  }
  {
    auto [value] = *stdexec::sync_wait(stdexec::when_all(
        channel.setValue(1) | stdexec::then([&]() { EXPECT_EQ(channel.size(), 1); }), channel.getValue()));

    EXPECT_EQ(value, 1);
    EXPECT_EQ(channel.size(), 0);
  }
}

TEST(Channel, SendCancel) {
  Channel<int, 1> channel;
  stdexec::sync_wait(channel.setValue(0));

  auto [res] =
      *stdexec::sync_wait(exec::when_any(stdexec::just(true), channel.setValue(0) | stdexec::then([]() {
                                                                EXPECT_TRUE(false);
                                                                return false;
                                                              })));

  EXPECT_TRUE(res);
  EXPECT_EQ(channel.size(), 1);
}

TEST(Channel, GetCancel) {
  Channel<int, 1> channel;

  auto [res] = *stdexec::sync_wait(exec::when_any(
      stdexec::just(true), channel.getValue() | stdexec::then([](auto /*unused*/) { return false; })));

  EXPECT_TRUE(res);
  EXPECT_EQ(channel.size(), 0);
}

TEST(Channel, SendRecvOrder) {
  Channel<int, 3> channel;

  stdexec::sync_wait(channel.setValue(0));
  stdexec::sync_wait(channel.setValue(1));
  stdexec::sync_wait(channel.setValue(2));
  EXPECT_EQ(channel.size(), 3);

  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 0);
  }
  EXPECT_EQ(channel.size(), 2);
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 1);
  }
  EXPECT_EQ(channel.size(), 1);
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 2);
  }
  EXPECT_EQ(channel.size(), 0);
  stdexec::sync_wait(channel.setValue(1));
  stdexec::sync_wait(channel.setValue(2));
  EXPECT_EQ(channel.size(), 2);
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 1);
  }
  EXPECT_EQ(channel.size(), 1);
  {
    auto [value] = *stdexec::sync_wait(channel.getValue());
    EXPECT_EQ(value, 2);
  }
  EXPECT_EQ(channel.size(), 0);
}

TEST(Channel, SendRecvParallel) {
  Channel<std::size_t, 3> channel;
  static constexpr std::size_t NUMBER_OF_ITERATIONS = 100;

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

}  // namespace heph::concurrency
// NOLINTEND(bugprone-unchecked-optional-access)
