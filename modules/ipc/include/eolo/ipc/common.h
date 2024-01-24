//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

namespace eolo::ipc {
struct Config {
  enum Mode { PEER, CLIENT };
  std::string topic;
  bool enable_shared_memory = false;  //! NOTE: With shared-memory enabled, the publisher still uses the
                                      //! network transport layer to notify subscribers of the shared-memory
                                      //! segment to read. Therefore, for very small messages, shared -
                                      //! memory transport could be less efficient than using the default
                                      //! network transport to directly carry the payload.
  Mode mode = PEER;
  // NOLINTNEXTLINE(readability-redundant-string-init) otherwise we need to specify in constructor
  std::string router = "";  //! If specified connect to the given router endpoint.
  std::size_t cache_size = 0;
};

struct MessageMetadata {
  // TODO: convert this to a uuid
  std::string sender_id;
  std::chrono::nanoseconds timestamp{};
  std::size_t sequence_id{};
};

}  // namespace eolo::ipc
