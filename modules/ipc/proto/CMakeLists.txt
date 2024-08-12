#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

set(SOURCES
  hephaestus/ipc/zenoh/action_server/proto/types.proto
)

define_module_proto_library(NAME action_server_types_proto SOURCES ${SOURCES})
