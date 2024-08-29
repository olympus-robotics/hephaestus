//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#pragma once

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/common.h"

namespace heph::ipc {

void appendIPCProgramOption(cli::ProgramDescription& program_description);

[[nodiscard]] auto parseIPCProgramOptions(const heph::cli::ProgramOptions& args)
    -> std::pair<Config, TopicConfig>;

}  // namespace heph::ipc
