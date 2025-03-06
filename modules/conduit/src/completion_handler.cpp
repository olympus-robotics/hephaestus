
//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <hephaestus/conduit/completion_handler.h>
#include <hephaestus/conduit/context.h>

namespace heph::conduit {

auto CompletionHandler::getSqe() const -> io_uring_sqe* {
  return context->getSqe();
}

}  // namespace heph::conduit
