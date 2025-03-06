
//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

// Forward declare to reduce header noise...
struct io_uring_cqe;
struct io_uring_sqe;

namespace heph::conduit {
struct Context;
struct CompletionHandler {
  virtual ~CompletionHandler() = default;
  virtual void handle(io_uring_cqe* cqe) = 0;

  explicit CompletionHandler(Context* context) noexcept : context(context) {
  }

  auto getSqe() const -> io_uring_sqe*;

  Context* context{ nullptr };
};
}  // namespace heph::conduit
