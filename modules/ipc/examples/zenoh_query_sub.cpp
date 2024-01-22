//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <print>
#include <thread>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/cli/program_options.h"
#include "eolo/ipc/zenoh/utils.h"
#include "eolo/serdes/serdes.h"
#include "eolo/types/pose.h"
#include "eolo/types_protobuf/pose.h"

void dataHandler(const z_sample_t* sample, void* arg) {
  (void)arg;
  z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);

  int counter = 0;
  if (sample->attachment.data != nullptr) {
    const auto& attachment = static_cast<const zenohc::AttachmentView&>(sample->attachment);
    auto counter_str = std::string{ attachment.get("msg_counter").as_string_view() };
    counter = std::stoi(counter_str);
  }

  eolo::types::Pose pose;
  const auto buffer = std::span<const std::byte>{ reinterpret_cast<const std::byte*>(sample->payload.start),
                                                  sample->payload.len };
  eolo::serdes::deserialize(buffer, pose);
  std::println(">> Time: {}. Topic {}. Counter: {}. Reeived {}",
               eolo::ipc::zenoh::toChrono(sample->timestamp.time), z_loan(keystr), counter,
               pose.position.transpose());

  z_drop(z_move(keystr));
}

auto main(int argc, const char* argv[]) -> int {
  try {
    static constexpr auto DEFAULT_KEY = "eolo/ipc/example/zenoh/put";

    auto desc = eolo::cli::ProgramDescription("Subscriber listening for data on specified key");
    desc.defineOption<std::string>("key", "Key expression", DEFAULT_KEY);

    const auto args = std::move(desc).parse(argc, argv);
    const auto key = args.getOption<std::string>("key");

    zenohc::Config config;

    std::println("Opening session...");
    auto session = eolo::ipc::zenoh::expect(open(std::move(config)));

    std::println("Declaring Subscriber on '{}'", key);

    const auto sub_opts = ze_querying_subscriber_options_default();
    z_owned_closure_sample_t callback = z_closure(dataHandler);
    auto sub =
        ze_declare_querying_subscriber(session.loan(), z_keyexpr(key.c_str()), z_move(callback), &sub_opts);
    eolo::throwExceptionIf<eolo::FailedZenohOperation>(!z_check(sub), "failed to create zenoh sub");

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
