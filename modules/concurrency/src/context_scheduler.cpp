//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/context_scheduler.h"

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

namespace heph::concurrency {
void TaskDispatchOperation::handleCompletion(::io_uring_cqe* /*cqe*/) {
  self->start();
}
}  // namespace heph::concurrency
