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
  Channel<int, 1> c;

  EXPECT_EQ(c.size(), 0);

  {
    auto [value] = *stdexec::sync_wait(stdexec::when_all(c.getValue(), c.setValue(1) | stdexec::then([&]() {
                                                                         // The size is zero here since the
                                                                         // getValue awaiter will be
                                                                         // immediately invoked before this
                                                                         // set completes
                                                                         EXPECT_EQ(c.size(), 0);
                                                                       })));

    EXPECT_EQ(value, 1);
    EXPECT_EQ(c.size(), 0);
  }
  {
    auto [value] = *stdexec::sync_wait(
        stdexec::when_all(c.setValue(1) | stdexec::then([&]() { EXPECT_EQ(c.size(), 1); }), c.getValue()));

    EXPECT_EQ(value, 1);
    EXPECT_EQ(c.size(), 0);
  }
}

TEST(Channel, SendCancel) {
  Channel<int, 1> c;
  stdexec::sync_wait(c.setValue(0));

  auto [res] = *stdexec::sync_wait(exec::when_any(stdexec::just(true), c.setValue(0) | stdexec::then([]() {
                                                                         EXPECT_TRUE(false);
                                                                         return false;
                                                                       })));

  EXPECT_TRUE(res);
  EXPECT_EQ(c.size(), 1);
}

TEST(Channel, GetCancel) {
  Channel<int, 1> c;

  auto [res] = *stdexec::sync_wait(exec::when_any(
      stdexec::just(true), c.getValue() | stdexec::then([](auto /*unused*/) { return false; })));

  EXPECT_TRUE(res);
  EXPECT_EQ(c.size(), 0);
}

TEST(Channel, SendRecvOrder) {
  Channel<int, 3> c;

  stdexec::sync_wait(c.setValue(0));
  stdexec::sync_wait(c.setValue(1));
  stdexec::sync_wait(c.setValue(2));
  EXPECT_EQ(c.size(), 3);

  {
    auto [value] = *stdexec::sync_wait(c.getValue());
    EXPECT_EQ(value, 0);
  }
  EXPECT_EQ(c.size(), 2);
  {
    auto [value] = *stdexec::sync_wait(c.getValue());
    EXPECT_EQ(value, 1);
  }
  EXPECT_EQ(c.size(), 1);
  {
    auto [value] = *stdexec::sync_wait(c.getValue());
    EXPECT_EQ(value, 2);
  }
  EXPECT_EQ(c.size(), 0);
  stdexec::sync_wait(c.setValue(1));
  stdexec::sync_wait(c.setValue(2));
  EXPECT_EQ(c.size(), 2);
  {
    auto [value] = *stdexec::sync_wait(c.getValue());
    EXPECT_EQ(value, 1);
  }
  EXPECT_EQ(c.size(), 1);
  {
    auto [value] = *stdexec::sync_wait(c.getValue());
    EXPECT_EQ(value, 2);
  }
  EXPECT_EQ(c.size(), 0);
}

TEST(Channel, SendRecvParallel) {
  Channel<std::size_t, 3> c;
  static constexpr std::size_t NUMBER_OF_ITERATIONS = 100;

  std::thread producer{ [&]() {
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      stdexec::sync_wait(c.setValue(i));
    }
  } };
  std::thread consumer{ [&]() {
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      auto [res] = *stdexec::sync_wait(c.getValue());
      EXPECT_EQ(res, i);
    }
  } };

  producer.join();
  consumer.join();
}

}  // namespace heph::concurrency
// NOLINTEND(bugprone-unchecked-optional-access)
