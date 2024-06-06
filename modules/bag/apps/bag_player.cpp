//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <csignal>
#include <exception>
#include <filesystem>
#include <future>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <mcap/reader.hpp>

#include "hephaestus/bag/zenoh_player.h"
#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/program_options.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

auto main(int argc, const char* argv[]) -> int {
  heph::utils::StackTrace stack_trace;

  try {
    auto desc = heph::cli::ProgramDescription("Playback a bag to zenoh topics");
    heph::ipc::appendIPCProgramOption(desc);
    desc.defineOption<std::filesystem::path>("input_bag", 'i', "output file where to write the bag")
        .defineFlag("wait_for_readers_to_connect", 'w',
                    "Wait for readers to connect before starting playback");
    const auto args = std::move(desc).parse(argc, argv);
    auto input_file = args.getOption<std::filesystem::path>("input_bag");
    auto wait_for_readers_to_connect = args.getOption<bool>("wait_for_readers_to_connect");
    auto [config, _] = heph::ipc::parseIPCProgramOptions(args);

    LOG(INFO) << fmt::format("Reading bag file: {}", input_file.string());

    heph::throwExceptionIf<heph::InvalidDataException>(
        !std::filesystem::exists(input_file),
        fmt::format("input bag file {} doesn't exist", input_file.string()));
    auto bag_reader = std::make_unique<mcap::McapReader>();
    const auto status = bag_reader->open(input_file.string());
    if (status.code != mcap::StatusCode::Success) {
      fmt::print(stderr, "Failed to open bag file: {}", status.message);
      std::exit(1);
    }

    heph::bag::ZenohPlayerParams params{ .session = heph::ipc::zenoh::createSession(std::move(config)),
                                         .bag_reader = std::move(bag_reader),
                                         .wait_for_readers_to_connect = wait_for_readers_to_connect };
    auto zenoh_player = heph::bag::ZenohPlayer::create(std::move(params));
    zenoh_player.start().get();

    heph::utils::InterruptHandlerOrAppComplete::wait(zenoh_player);

  } catch (std::exception& e) {
    fmt::println("Failed with exception: {}", e.what());
    std::exit(1);
  }
}
