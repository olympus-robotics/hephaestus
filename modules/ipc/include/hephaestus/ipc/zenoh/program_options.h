//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#pragma once

#include <string>
#include <tuple>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

static constexpr auto DEFAULT_TOPIC = "";
void appendProgramOption(cli::ProgramDescription& program_description,
                         const std::string& default_topic = DEFAULT_TOPIC);

[[nodiscard]] auto parseProgramOptions(const heph::cli::ProgramOptions& args)
    -> std::tuple<Config, TopicConfig, TopicFilterParams>;

}  // namespace heph::ipc::zenoh
