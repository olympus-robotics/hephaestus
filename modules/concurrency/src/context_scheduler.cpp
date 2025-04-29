//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/context_scheduler.h"

#include <stdexec/__detail/__env.hpp>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/context.h"

namespace heph::concurrency {
[[nodiscard]] auto ContextEnv::query(stdexec::get_stop_token_t /*ignore*/) const noexcept
    -> stdexec::inplace_stop_token {
  return self->getStopToken();
}

void TaskDispatchOperation::handleCompletion() const {
  self->start();
}
}  // namespace heph::concurrency
