//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/concurrency/io_ring/stoppable_io_ring_operation.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
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
  EXPECT_FALSE(ring.isRunning());

  EXPECT_NO_THROW(ring.run());
  EXPECT_FALSE(ring.isRunning());
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
  EXPECT_FALSE(ring.isRunning());
  EXPECT_NO_THROW(ring.run());
  EXPECT_FALSE(ring.isRunning());

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

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  ring_ptr->requestStop();
  runner.join();
  EXPECT_GE(completions, 0);
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
}  // namespace heph::concurrency::io_ring::tests
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
