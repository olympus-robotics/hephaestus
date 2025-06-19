//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/context_scheduler.h"

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/__detail/__env.hpp>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/context.h"

namespace heph::concurrency {
[[nodiscard]] auto ContextEnv::query(stdexec::get_stop_token_t /*ignore*/) const noexcept
    -> stdexec::inplace_stop_token {
  return self->getStopToken();
}

[[nodiscard]] auto ContextEnv::query(GetContextT /*ignore*/) const noexcept -> Context& {
  return *self;
}

void TaskDispatchOperation::handleCompletion(::io_uring_cqe* /*cqe*/) {
  self->start();
}
}  // namespace heph::concurrency
