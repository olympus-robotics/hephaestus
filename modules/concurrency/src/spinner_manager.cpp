//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner_manager.h"

#include <algorithm>
#include <atomic>
#include <future>
#include <utility>
#include <vector>

#include <fmt/core.h>

#include "hephaestus/concurrency/spinner.h"

namespace heph::concurrency {
SpinnerManager::SpinnerManager(std::vector<Spinner*> spinners) : spinners_{ std::move(spinners) } {
}

void SpinnerManager::startAll() {
  std::ranges::for_each(spinners_, [](auto* runner) { runner->start(); });
}

void SpinnerManager::waitAll() {
  std::ranges::for_each(spinners_, [](auto* runner) { runner->wait(); });
}

void SpinnerManager::waitAny() {
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  wait_futures_.reserve(spinners_.size());
  for (auto* runner : spinners_) {
    auto future = std::async(std::launch::async, [runner, &flag]() {
      runner->wait();
      flag.test_and_set();
      flag.notify_all();
    });
    wait_futures_.push_back(std::move(future));
  }

  flag.wait(false);
}

void SpinnerManager::stopAll() {
  std::vector<std::future<void>> futures;
  futures.reserve(spinners_.size());

  std::ranges::for_each(spinners_,
                        [&futures](auto* runner) { futures.push_back(std::move(runner->stop())); });

  // If we called `waitAny`, we need to wait for the remaining futures.
  std::ranges::for_each(wait_futures_, [](auto& future) { future.get(); });

  std::ranges::for_each(futures, [](auto& f) { f.get(); });
}
}  // namespace heph::concurrency
