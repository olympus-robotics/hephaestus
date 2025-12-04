//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <concepts>
#include <utility>

#include <exec/task.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"

// NOLINTBEGIN(bugprone-unchecked-optional-access)
namespace heph::concurrency {

TEST(AnySender, JustVoid) {
  // Test that constructor rejects non-void senders
  static_assert(std::constructible_from<AnySender<void>, decltype(stdexec::just())>);
  static_assert(!std::constructible_from<AnySender<void>, decltype(stdexec::just(0))>);

  // Test that void senders work
  {
    AnySender<void> sender{ stdexec::just() };
    auto res = stdexec::sync_wait(std::move(sender));
    EXPECT_TRUE(res.has_value());
  }

  // Test that cancellation works
  {
    AnySender<void> sender{ stdexec::just_stopped() };
    auto res = stdexec::sync_wait(std::move(sender));
    EXPECT_FALSE(res.has_value());
  }

  // Test compatibility with coroutines
  {
    auto coroutine = []() -> exec::task<void> { co_return co_await AnySender<void>{ stdexec::just() }; };
    auto res = stdexec::sync_wait(coroutine());
    EXPECT_TRUE(res.has_value());
  }
  {
    auto coroutine = []() -> exec::task<void> {
      co_return co_await AnySender<void>{ stdexec::just_stopped() };
    };
    auto res = stdexec::sync_wait(coroutine());
    EXPECT_FALSE(res.has_value());
  }

  {
    auto coroutine = []() -> exec::task<void> { co_return; };
    AnySender<void> sender{ coroutine() };
    auto res = stdexec::sync_wait(std::move(sender));
    EXPECT_TRUE(res.has_value());
  }
}

TEST(AnySender, JustValue) {
  // Test that constructor rejects non-void senders
  static_assert(!std::constructible_from<AnySender<int>, decltype(stdexec::just())>);
  static_assert(std::constructible_from<AnySender<int>, decltype(stdexec::just(1))>);

  // Test that int senders work
  {
    AnySender<int> sender{ stdexec::just(1) };
    auto res = stdexec::sync_wait(std::move(sender));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(std::get<0>(*res), 1);
  }

  // Test that cancellation works
  {
    AnySender<int> sender{ stdexec::just_stopped() };
    auto res = stdexec::sync_wait(std::move(sender));
    EXPECT_FALSE(res.has_value());
  }

  // Test compatibility with coroutines
  {
    auto coroutine = []() -> exec::task<int> { co_return co_await AnySender<int>{ stdexec::just(1) }; };
    auto res = stdexec::sync_wait(coroutine());
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(std::get<0>(*res), 1);
  }
  {
    auto coroutine = []() -> exec::task<int> {
      co_return co_await AnySender<int>{ stdexec::just_stopped() };
    };
    auto res = stdexec::sync_wait(coroutine());
    EXPECT_FALSE(res.has_value());
  }

  {
    auto coroutine = []() -> exec::task<int> { co_return 1; };
    AnySender<int> sender{ coroutine() };
    auto res = stdexec::sync_wait(std::move(sender));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(std::get<0>(*res), 1);
  }
}

TEST(AnySender, Composable) {
  bool triggered = false;
  stdexec::sync_wait(AnySender<void>{ stdexec::just() } | stdexec::then([&]() { triggered = true; }));
  EXPECT_TRUE(triggered);
}
}  // namespace heph::concurrency
// NOLINTEND(bugprone-unchecked-optional-access)
