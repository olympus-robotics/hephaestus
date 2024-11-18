//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinners_manager.h"

#include <algorithm>
#include <atomic>
#include <future>
#include <utility>
#include <vector>

#include "hephaestus/concurrency/spinner.h"

namespace heph::concurrency {
SpinnersManager::SpinnersManager(std::vector<Spinner*> spinners) : spinners_{ std::move(spinners) } {
  std::ranges::for_each(spinners_, [this](auto* spinner) {
    spinner->setTerminationCallback([this]() {
      if (!termination_flag_.test_and_set()) {
        termination_flag_.notify_all();
      }
    });
  });
}

void SpinnersManager::startAll() {
  std::ranges::for_each(spinners_, [](auto* spinner) { spinner->start(); });
}

void SpinnersManager::waitAll() {
  std::ranges::for_each(spinners_, [](auto* spinner) { spinner->wait(); });
}

void SpinnersManager::waitAny() {
  termination_flag_.wait(false);
}

void SpinnersManager::stopAll() {
  std::vector<std::future<void>> futures;
  futures.reserve(spinners_.size());
  std::ranges::for_each(spinners_,
                        [&futures](auto* spinner) { futures.push_back(std::move(spinner->stop())); });

  std::ranges::for_each(futures, [](auto& f) { f.get(); });
}
}  // namespace heph::concurrency
