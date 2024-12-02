//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <vector>

#include "hephaestus/concurrency/spinner.h"

namespace heph::concurrency {

/// @brief SpinnersManager allows to orchestrate the execution of multiple spinners.
/// The main feature is `waitAny` which allows to unblock as soon as one of the spinners is done.
/// This allows to catch errors in spinner execution as soon as possible and stop the others.
/// NOTE: right now we do not have any concept of error for the spinners: we cannot know if a spinner
/// terminated with an error or not. If an exception is thrown inside the runner, it will be re-throwed when
/// we call runner.stop().get(). We leave it to the user to handle it outside of the runner manager.
/// NOTE: this logic is quite generic and can be extended to any type of object that has `wait()` and `stop()`
/// methods. To be faitful to the principle of implement only what you need now, we limit the scope to
/// spinners and consider to expand the scope when an use case arises.
class SpinnersManager {
public:
  explicit SpinnersManager(std::vector<Spinner*> spinners);

  void startAll();
  /// @brief blocks until all spinners are finished. If a single spinner throws an exception, it will not be
  /// propagated as the other spinners are still blocking.
  void waitAll();
  /// @brief wait until any spinner terminates or throws an exception, which allows for immediate exception
  /// handling. Note that the exceptions will be re-thrown when calling `stopAll`.
  void waitAny();
  void stopAll();

private:
  std::vector<Spinner*> spinners_;

  std::atomic_flag termination_flag_ = ATOMIC_FLAG_INIT;
};

}  // namespace heph::concurrency
