//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <future>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/containers/blocking_queue.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::containers::tests {

TEST(BlockingQueue, Failures) {
  EXPECT_NO_THROW(BlockingQueue<int>{ 0 };);
}

TEST(BlockingQueue, Push) {
  constexpr int QUEUE_SIZE = 2;
  BlockingQueue<int> block_queue(QUEUE_SIZE);
  EXPECT_THAT(block_queue, IsEmpty());

  auto success = block_queue.tryPush(1);
  EXPECT_TRUE(success);
  EXPECT_THAT(block_queue, SizeIs(1));

  success = block_queue.tryPush(2);
  EXPECT_TRUE(success);
  EXPECT_THAT(block_queue, SizeIs(2));

  success = block_queue.tryPush(3);
  EXPECT_FALSE(success);
  EXPECT_THAT(block_queue, SizeIs(2));

  auto element_dropped = block_queue.forcePush(4);
  EXPECT_TRUE(element_dropped);
  if (element_dropped.has_value()) {
    EXPECT_EQ(*element_dropped, 1);
  }
  EXPECT_THAT(block_queue, SizeIs(2));

  auto data = block_queue.tryPop();
  EXPECT_EQ(data, 2);
  EXPECT_THAT(block_queue, SizeIs(1));

  data = block_queue.tryPop();
  EXPECT_EQ(data, 4);
  EXPECT_THAT(block_queue, IsEmpty());

  data = block_queue.tryPop();
  EXPECT_THAT(data, Eq(std::nullopt));
  EXPECT_THAT(block_queue, IsEmpty());

  auto future = std::async([&block_queue]() { return block_queue.waitAndPop(); });
  EXPECT_TRUE(block_queue.tryPush(1));
  EXPECT_EQ(future.get(), 1);

  future = std::async([&block_queue]() { return block_queue.waitAndPop(); });
  block_queue.stop();
  future.get();
  EXPECT_TRUE(true);
}

TEST(BlockingQueue, WaitPush) {
  constexpr int QUEUE_SIZE = 1;
  BlockingQueue<std::string> block_queue(QUEUE_SIZE);
  const std::string message = "hello";
  block_queue.waitAndPush(message);
  auto future = std::async([&block_queue]() {
    std::string msg = "hello again";
    block_queue.waitAndPush(std::move(msg));
  });
  auto data = block_queue.tryPop();
  EXPECT_TRUE(data);
  if (data.has_value()) {
    EXPECT_EQ(*data, message);
  }
  future.get();
  data = block_queue.tryPop();
  EXPECT_TRUE(data);
  if (data.has_value()) {
    EXPECT_EQ(*data, "hello again");
  }
}

TEST(BlockingQueue, TryEmplace) {
  constexpr int QUEUE_SIZE = 1;
  BlockingQueue<std::tuple<int, std::string, double>> block_queue(QUEUE_SIZE);
  EXPECT_TRUE(block_queue.tryEmplace(1, "hello", 1.0));
  EXPECT_FALSE(block_queue.tryEmplace(0, "bye", 0.0));
  auto data = block_queue.tryPop();
  EXPECT_TRUE(data);
  if (data.has_value()) {
    EXPECT_EQ(std::get<2>(*data), 1.0);
  }
}

TEST(BlockingQueue, ForceEmplace) {
  constexpr int QUEUE_SIZE = 1;
  BlockingQueue<std::tuple<int, std::string, double>> block_queue(QUEUE_SIZE);

  EXPECT_TRUE(block_queue.tryEmplace(1, "hello", 1.0));
  auto element_dropped = block_queue.forceEmplace(0, "bye", 0.0);
  EXPECT_TRUE(element_dropped);
  auto expected_element_dropped = std::tuple{ 1, "hello", 1.0 };
  if (element_dropped.has_value()) {
    EXPECT_EQ(*element_dropped, expected_element_dropped);
  }
  auto data = block_queue.tryPop();
  EXPECT_TRUE(data);
  if (data.has_value()) {
    EXPECT_EQ(std::get<2>(*data), 0.0);
  }
}

TEST(BlockingQueue, WaitEmplace) {
  constexpr int QUEUE_SIZE = 1;
  BlockingQueue<std::tuple<int, std::string, double>> block_queue(QUEUE_SIZE);

  block_queue.waitAndEmplace(1, "hello", 1.0);
  auto future = std::async([&block_queue]() { block_queue.waitAndEmplace(2, "hello", 1.0); });
  auto data = block_queue.tryPop();
  EXPECT_TRUE(data);
  if (data.has_value()) {
    EXPECT_EQ(std::get<0>(*data), 1);
  }
  future.get();
  data = block_queue.tryPop();
  EXPECT_TRUE(data);
  if (data.has_value()) {
    EXPECT_EQ(std::get<0>(*data), 2);
  }
}

TEST(BlockingQueue, LargeQueue) {
  BlockingQueue<double> block_queue{ std::numeric_limits<size_t>::max() };

  static constexpr double A_NUMBER = 42;
  auto element_dropped = block_queue.forceEmplace(A_NUMBER);
  EXPECT_FALSE(element_dropped);
  EXPECT_TRUE(block_queue.tryEmplace(A_NUMBER));
  block_queue.waitAndEmplace(A_NUMBER);
  element_dropped = block_queue.forcePush(A_NUMBER);
  EXPECT_FALSE(element_dropped);
  EXPECT_TRUE(block_queue.tryPush(A_NUMBER));
  block_queue.waitAndPush(A_NUMBER);

  EXPECT_EQ(block_queue.waitAndPop(), A_NUMBER);
  auto res = block_queue.tryPop();
  EXPECT_TRUE(res);
  if (res.has_value()) {
    EXPECT_EQ(*res, A_NUMBER);
  }
  EXPECT_EQ(block_queue.waitAndPop(), A_NUMBER);
  EXPECT_EQ(block_queue.waitAndPop(), A_NUMBER);
  EXPECT_EQ(block_queue.waitAndPop(), A_NUMBER);
  EXPECT_EQ(block_queue.waitAndPop(), A_NUMBER);

  res = block_queue.tryPop();
  EXPECT_FALSE(res);
}

TEST(BlockingQueue, Restart) {
  BlockingQueue<double> block_queue{ 2 };
  block_queue.waitAndPush(0.);
  block_queue.restart();
  EXPECT_THAT(block_queue, IsEmpty());
}

TEST(BlockingQueue, WaitForEmpty) {
  BlockingQueue<double> block_queue{ 2 };
  block_queue.waitAndPush(0.);
  auto wait_future = std::async(std::launch::async, [&block_queue]() { block_queue.waitForEmpty(); });
  std::ignore = block_queue.waitAndPopAll();
  wait_future.get();
}

}  // namespace heph::containers::tests
