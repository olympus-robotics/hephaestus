//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================
#include "eolo/ipc/zenoh/query.h"

#include <barrier>
#include <mutex>

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

namespace eolo::ipc::zenoh {

auto query(zenohc::Session& session, const std::string& topic,
           const std::string& value) -> std::vector<QueryResponse> {
  std::barrier sync_point(2);
  std::vector<QueryResponse> results;
  std::mutex mutex;
  const auto reply_cb = [&results, &mutex](zenohc::Reply&& reply) {
    const auto r = std::move(reply).get();
    if (std::holds_alternative<zenohc::ErrorMessage>(r)) {
      return;
    }

    const auto sample = std::get<zenohc::Sample>(r);
    auto topic = std::string{ sample.get_keyexpr().as_string_view() };
    auto result = std::string{ sample.get_payload().as_string_view() };
    fmt::println("Timestamp: {}", sample.get_timestamp().get_time());

    std::unique_lock<std::mutex> lock(mutex);
    results.emplace_back(std::move(topic), std::move(result));
  };

  auto on_done = [&sync_point] { sync_point.arrive_and_drop(); };

  auto opts = zenohc::GetOptions();
  opts.set_value(zenohc::Value{ value });
  opts.set_target(Z_QUERY_TARGET_ALL);  // query all matching queryables
  session.get(topic, "", { reply_cb, on_done }, opts);

  sync_point.arrive_and_wait();  // wait until reply callback is triggered

  return results;
}
}  // namespace eolo::ipc::zenoh
