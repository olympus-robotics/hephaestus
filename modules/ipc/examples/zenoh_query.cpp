//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================
#include <barrier>

#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/utils.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Query");
    const auto args = std::move(desc).parse(argc, argv);

    auto config = parseArgs(args);

    auto session = eolo::ipc::zenoh::createSession(config);
    std::println("Opening session: {}", eolo::ipc::zenoh::toString(session->info_zid()));

    std::barrier sync_point(2);

    // define callback to trigger on response
    const auto reply_cb = [](zenohc::Reply&& reply) {
      try {
        std::println("Callback");
        const auto sample = eolo::ipc::zenoh::expect<zenohc::Sample>(reply.get());
        const auto& keystr = sample.get_keyexpr();
        std::println(">> Received ('{}': '{}')", keystr.as_string_view(),
                     sample.get_payload().as_string_view());
      } catch (const std::exception& ex) {
        std::println("Exception in reply callback: {}", ex.what());
      }
    };

    auto on_done = [&sync_point] { sync_point.arrive_and_drop(); };

    // prepare and dispatch request
    auto opts = zenohc::GetOptions();
    opts.set_value("");
    opts.set_target(Z_QUERY_TARGET_ALL);  // query all matching queryables
    session->get(config.topic, "", { reply_cb, on_done }, opts);

    sync_point.arrive_and_wait();  // wait until reply callback is triggered

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
