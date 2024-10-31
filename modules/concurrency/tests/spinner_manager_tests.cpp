//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <span>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/spinner.h"

namespace heph::concurrency::tests {

/// @brief SpinnerManager allows to orchestrate the execution of multiple spinners.
/// The main feature is `waitAny` which allows to unblock as soon as one of the spinners is done.
/// This allows to catch errors in spinner execution as soon as possible and stop the others.
/// NOTE: right now we do not have any concept of error for the spinners: we cannot know if a spinner
/// terminated with an error or not. If an exception is thrown inside the runner, it will be re-throwed when
/// we call runner.stop().get(). We leave it to the user to handle it outside of the runner manager.
/// NOTE: this logic is quite generic and can be extended to any type of object that has `wait()` and `stop()`
/// methods. To be faitful to the principle of implement only what you need now, we limit the scope to
/// spinners and consider to expand the scope when an use case arises.
class SpinnerManager {
public:
  explicit SpinnerManager(std::span<Spinner*> spinners) : spinners_{ spinners } {
  }

  void startAll() {
    std::ranges::for_each(spinners_, [](auto* runner) { runner->start(); });
  }

  void waitAll() {
    std::ranges::for_each(spinners_, [](auto* runner) { runner->wait(); });
  }

  void waitAny() {
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

  void stopAll() {
    std::vector<std::future<void>> futures;
    futures.reserve(spinners_.size());
    std::ranges::for_each(spinners_, [&futures](auto* runner) { futures.push_back(runner->stop()); });

    std::ranges::for_each(wait_futures_, [](auto& future) { future.get(); });

    std::ranges::for_each(futures, [](auto& f) { f.get(); });
  }

private:
  std::span<Spinner*> spinners_;

  std::vector<std::future<void>> wait_futures_;
};

TEST(RunnerManager, Empty) {
  SpinnerManager runner_manager{ {} };
}

}  // namespace heph::concurrency::tests
