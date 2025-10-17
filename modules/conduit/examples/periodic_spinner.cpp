//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <thread>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <fmt/format.h>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/periodic.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/utils/signal_handler.h"

// APIDOC: begin
namespace {
// Spinner function:
// This function is a coroutine, which calls the periodic in an endless loop.
// The print statement will be executed every 10 seconds.
auto spinner(heph::conduit::SchedulerT scheduler) -> exec::task<void> {
  heph::conduit::Periodic periodic;

  // Setup our period duration
  static constexpr std::chrono::seconds PERIOD_DURATION{ 10 };
  periodic.setPeriodDuration(PERIOD_DURATION);

  auto last_spin_time = heph::conduit::ClockT::now();
  while (true) {
    // Call trigger, this will resume the coroutine once the period duration
    // is elapsed.
    co_await periodic.trigger(scheduler);

    // Print the duration it took since last execution. Note that the first iteration
    // will finish right away.
    auto now = heph::conduit::ClockT::now();
    fmt::println("Time elapsed since last spin: {:.2%Ss}", now - last_spin_time);
    // Adding additional work or waiting doesn't influence the spinning time
    co_await scheduler.scheduleAfter(std::chrono::seconds{ 2 });
    last_spin_time = now;
  }
}
}  // namespace

auto main() -> int {
  // Setup our context on which we schedule our work
  heph::concurrency::Context context{ {} };

  // The spin function needs to be executed inside a scope.
  exec::async_scope scope;

  // Spawn the spinner: As `spinner` is a coroutine, it will suspend right away to wait for
  // the context to start.
  scope.spawn(spinner(context.scheduler()));

  fmt::println("Starting Spinner, exit by pressing ctrl+c");
  heph::utils::TerminationBlocker::registerInterruptCallback([&context]() { context.requestStop(); });
  context.run();
}
// APIDOC: end
