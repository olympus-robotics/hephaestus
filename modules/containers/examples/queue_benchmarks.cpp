//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include <benchmark/benchmark.h>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/containers/fixed_circular_buffer.h"

namespace {
inline constexpr std::size_t PAYLOAD_SIZE = 1024;
using PayloadT = std::array<std::size_t, PAYLOAD_SIZE>;
void blockingQueueSingleThread(benchmark::State& state) {
  heph::containers::BlockingQueue<PayloadT> queue(1);

  for ([[maybe_unused]] auto _ : state) {
    (void)queue.tryPush(PayloadT{});
    auto payload = queue.tryPop();
    benchmark::DoNotOptimize(payload);
  }
  state.SetBytesProcessed(
      static_cast<std::int64_t>(static_cast<std::size_t>(state.iterations()) * sizeof(PayloadT)));
}
BENCHMARK(blockingQueueSingleThread)->UseRealTime();

void fixedCircularBufferSingleThread(benchmark::State& state) {
  heph::containers::FixedCircularBuffer<PayloadT, 1> queue;

  for ([[maybe_unused]] auto _ : state) {
    (void)queue.push(PayloadT{});
    auto payload = queue.pop();
    benchmark::DoNotOptimize(payload);
  }
  state.SetBytesProcessed(
      static_cast<std::int64_t>(static_cast<std::size_t>(state.iterations()) * sizeof(PayloadT)));
}
BENCHMARK(fixedCircularBufferSingleThread)->UseRealTime();

struct MultiThread : benchmark::Fixture {
  void SetUp(::benchmark::State& /*state*/) override {
  }
  void TearDown(::benchmark::State& /*state*/) override {
  }

  heph::containers::BlockingQueue<PayloadT> blocking_queue{ 1 };
  std::mutex mutex;
  heph::containers::FixedCircularBuffer<PayloadT, 1> fixed_buffer;
};

BENCHMARK_DEFINE_F(MultiThread, blockingQueue)(benchmark::State& state) {
  if (state.thread_index() == 0) {
    for ([[maybe_unused]] auto _ : state) {
      while (!blocking_queue.tryPush(PayloadT{})) {
        ;  // wait for the consumer
      }
    }
  }
  if (state.thread_index() == 1) {
    for ([[maybe_unused]] auto _ : state) {
      while (true) {
        auto payload = blocking_queue.tryPop();
        if (payload.has_value()) {
          break;
        }
      }
    }
  }
  state.SetBytesProcessed(
      static_cast<std::int64_t>(static_cast<std::size_t>(state.iterations()) * sizeof(PayloadT)));
}
BENCHMARK_REGISTER_F(MultiThread, blockingQueue)->Threads(2)->UseRealTime();

BENCHMARK_DEFINE_F(MultiThread, fixedCircularBufferMutex)(benchmark::State& state) {
  if (state.thread_index() == 0) {
    for ([[maybe_unused]] auto _ : state) {
      while (true) {
        const std::unique_lock lock{ mutex };
        if (fixed_buffer.push(PayloadT{})) {
          break;
        }
      }
    }
  }
  if (state.thread_index() == 1) {
    for ([[maybe_unused]] auto _ : state) {
      while (true) {
        const std::unique_lock lock{ mutex };
        auto payload = fixed_buffer.pop();
        if (payload.has_value()) {
          break;
        }
      }
    }
  }
  state.SetBytesProcessed(
      static_cast<std::int64_t>(static_cast<std::size_t>(state.iterations()) * sizeof(PayloadT)));
}
BENCHMARK_REGISTER_F(MultiThread, fixedCircularBufferMutex)->Threads(2)->UseRealTime();
}  // namespace

BENCHMARK_MAIN();
