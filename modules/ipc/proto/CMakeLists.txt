#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

set(SOURCES
    hephaestus/ipc/zenoh/proto/action_server_types.proto
)

define_module_proto_library(NAME action_server_types_proto SOURCES ${SOURCES})
