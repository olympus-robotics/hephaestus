//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::containers {

struct Dummy {
  Dummy* next{ nullptr };
};

TEST(IntrusiveFifoQueue, Empty) {
  IntrusiveFifoQueue<Dummy> queue;
  EXPECT_EQ(queue.size(), 0);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.dequeue(), nullptr);
}

TEST(IntrusiveFifoQueue, EnqueueDequeue) {
  IntrusiveFifoQueue<Dummy> queue;

  Dummy dummy1;
  Dummy dummy2;
  Dummy dummy3;
  Dummy dummy4;

  queue.enqueue(&dummy1);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);

  queue.enqueue(&dummy1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  queue.enqueue(&dummy2);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 2);
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy2);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);

  queue.enqueue(&dummy1);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  queue.enqueue(&dummy2);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.empty());
  queue.enqueue(&dummy3);
  EXPECT_EQ(queue.size(), 3);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.empty());
  queue.enqueue(&dummy4);
  EXPECT_EQ(queue.size(), 3);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy2);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy3);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy4);
  EXPECT_EQ(queue.size(), 0);
  EXPECT_TRUE(queue.empty());
}

class DummyPrivate {
private:
  friend struct IntrusiveFifoQueueAccess;
  [[maybe_unused]] DummyPrivate* next_{ nullptr };
};
TEST(IntrusiveFifoQueue, Access) {
  IntrusiveFifoQueue<DummyPrivate> queue;

  DummyPrivate dummy;
  queue.enqueue(&dummy);
  EXPECT_EQ(queue.dequeue(), &dummy);
}
}  // namespace heph::containers
