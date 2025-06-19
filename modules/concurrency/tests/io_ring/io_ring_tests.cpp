//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cerrno>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/concurrency/io_ring/stoppable_io_ring_operation.h"

namespace heph::concurrency::io_ring::tests {

TEST(IoRingTest, DefaultConstruction) {
  EXPECT_NO_THROW(const IoRing ring{ {} });
}

struct StopOperation : IoRingOperationBase {
  IoRing* ring{ nullptr };

  void prepare(::io_uring_sqe* sqe) final {
    ::io_uring_prep_nop(sqe);
  }

  void handleCompletion(::io_uring_cqe* cqe) final {
    EXPECT_EQ(cqe->res, 0);
    ring->requestStop();
  }
};

TEST(IoRingTest, startStop) {
  const IoRingConfig config;
  IoRing ring{ config };

  StopOperation stopper;
  stopper.ring = &ring;

  ring.submit(&stopper);
  EXPECT_FALSE(ring.getStopToken().stop_requested());

  EXPECT_NO_THROW(ring.run());
  EXPECT_TRUE(ring.getStopToken().stop_requested());

  // Running it agin should not panic, but return immediately.
  EXPECT_NO_THROW(ring.run());
  EXPECT_TRUE(ring.getStopToken().stop_requested());
}

struct DummyOperation : IoRingOperationBase {
  std::size_t* completions{ nullptr };

  void handleCompletion(::io_uring_cqe* cqe) final {
    EXPECT_EQ(cqe->res, 0);
    ++(*completions);
  }
};

TEST(IoRingTest, submit) {
  const IoRingConfig config;
  IoRing ring{ config };

  std::size_t completions = 0;
  std::vector<DummyOperation> ops(static_cast<std::size_t>(config.nentries * 3));

  for (DummyOperation& op : ops) {
    op.completions = &completions;
    EXPECT_NO_THROW(ring.submit(&op));
  }

  StopOperation stopper;
  stopper.ring = &ring;

  ring.submit(&stopper);
  EXPECT_FALSE(ring.getStopToken().stop_requested());
  EXPECT_NO_THROW(ring.run());
  EXPECT_TRUE(ring.getStopToken().stop_requested());

  EXPECT_TRUE(completions == static_cast<std::size_t>(config.nentries * 3));
}

TEST(IoRingTest, submitConcurrent) {
  IoRingConfig config;
  std::mutex mtx;
  std::condition_variable cv;
  IoRing* ring_ptr{ nullptr };

  std::size_t completions = 0;
  std::vector<DummyOperation> ops(static_cast<std::size_t>(config.nentries * 3));

  std::thread runner{ [&config, &mtx, &cv, &ring_ptr] {
    IoRing ring{ config };
    ring.run([&] {
      {
        const std::scoped_lock l{ mtx };
        ring_ptr = &ring;
      }
      cv.notify_all();
    });
  } };

  {
    std::unique_lock l{ mtx };
    cv.wait(l, [&ring_ptr] { return ring_ptr != nullptr; });
  }

  for (DummyOperation& op : ops) {
    op.completions = &completions;
    EXPECT_NO_THROW(ring_ptr->submit(&op));
  }

  ring_ptr->requestStop();
  runner.join();
  EXPECT_EQ(completions, static_cast<std::size_t>(config.nentries * 3));
}

struct TestOperation1T {
  void prepare(io_uring_sqe* sqe) {
    ::io_uring_prep_timeout(sqe, &ts, 0, 0);
  }
  static void handleCompletion(io_uring_cqe* /*cqe*/) {
    EXPECT_TRUE(false) << "completion handler should not get called";
  }
  void handleStopped() {
    stop_called = true;
  }
  bool stop_called{ false };
  static constexpr int HUGE_TIMEOUT_S = 60;
  __kernel_timespec ts{ .tv_sec = HUGE_TIMEOUT_S, .tv_nsec = 0 };  // NOLINT(misc-include-cleaner)
};
using TestOperation1 = StoppableIoRingOperation<TestOperation1T>;

struct StopTestOperation : IoRingOperationBase {
  void prepare(io_uring_sqe* sqe) final {
    ::io_uring_prep_nop(sqe);
  }
  void handleCompletion(io_uring_cqe* /*cqe*/) final {
    ring->requestStop();
  }
  IoRing* ring{ nullptr };
};

TEST(IoRingTest, stoppableOperation) {
  // 1. submit, 2. stop
  {
    IoRing ring{ {} };

    TestOperation1 test_operation1{ {}, ring, ring.getStopToken() };
    [[maybe_unused]] StopTestOperation stop_operation;
    stop_operation.ring = &ring;

    ring.submit(&test_operation1);
    ring.submit(&stop_operation);

    ring.run();

    EXPECT_TRUE(test_operation1.operation.stop_called);
  }
  // 1. stop, 2. submit
  {
    IoRing ring{ {} };

    TestOperation1 test_operation1{ {}, ring, ring.getStopToken() };
    [[maybe_unused]] StopTestOperation stop_operation;
    stop_operation.ring = &ring;

    ring.submit(&stop_operation);
    ring.submit(&test_operation1);

    ring.run();

    EXPECT_TRUE(test_operation1.operation.stop_called);
  }
}

TEST(IoRingTest, stoppableOperationConcurrent) {
  IoRingConfig config;
  std::mutex mtx;
  std::condition_variable cv;
  IoRing* ring_ptr{ nullptr };
  std::optional<IoRing> ring;

  std::vector<std::unique_ptr<TestOperation1>> ops;
  ops.reserve(static_cast<std::size_t>(config.nentries) * 3);

  std::thread runner{ [&config, &ops, &mtx, &cv, &ring, &ring_ptr] {
    ring.emplace(config);
    for (std::size_t i = 0; i != ops.capacity(); ++i) {
      ops.push_back(std::make_unique<TestOperation1>(TestOperation1T{}, *ring, ring->getStopToken()));
      EXPECT_NO_THROW(ring->submit(ops[i].get()));
    }

    ring->run([&] {
      {
        const std::scoped_lock l{ mtx };
        ring_ptr = &ring.value();
      }
      cv.notify_all();
    });
  } };

  {
    std::unique_lock l{ mtx };
    cv.wait(l, [&ring_ptr] { return ring_ptr != nullptr; });
  }

  ring_ptr->requestStop();
  runner.join();
  for (const auto& op : ops) {
    EXPECT_TRUE(op->operation.stop_called);
  }
}

}  // namespace heph::concurrency::io_ring::tests
