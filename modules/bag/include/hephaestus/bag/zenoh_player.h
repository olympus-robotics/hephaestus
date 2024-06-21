//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>

#include <mcap/reader.hpp>

#include "hephaestus/ipc/zenoh/session.h"

namespace heph::bag {

struct ZenohPlayerParams {
  ipc::zenoh::SessionPtr session;
  std::unique_ptr<mcap::McapReader> bag_reader;
  bool wait_for_readers_to_connect{ false };
};

class ZenohPlayer {
public:
  ~ZenohPlayer();

  [[nodiscard]] static auto create(ZenohPlayerParams params) -> ZenohPlayer;

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

  void wait() const;

private:
  explicit ZenohPlayer(ZenohPlayerParams params);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace heph::bag
