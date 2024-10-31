//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>
#include <vector>

#include "hephaestus/concurrency/spinner.h"

namespace heph::concurrency {

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
  explicit SpinnerManager(std::vector<Spinner*> spinners);

  void startAll();
  void waitAll();
  void waitAny();
  void stopAll();

private:
  std::vector<Spinner*> spinners_;

  std::vector<std::future<void>> wait_futures_;
};

}  // namespace heph::concurrency
